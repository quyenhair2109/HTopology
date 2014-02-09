//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "HTopology.h"
#include "HMessage_m.h"
#include <rpcmacros.h>

Define_Module(HTopology);

/*
 * TODO
 * MAJOR CONCERN NOW
 *  -> SCHEDULING (in rescue mode over a set of nodes)
 *      Can  be taken care of, if only RANKING works fine
 *      => just schedule the segments in one-by-one in the increasing order of timeRemainingInDeadline
 *          in the ranked rescue set
 *  -> ACTIVENESS of Nodes (liveness of nodes & keep alive messages)
 *      Activeness (check in case of abrupt node failures)
 *      - can be associated with a timeout to the videoSegmentCall (i.e. it doesn't reach the node)
 *      - ALTERNATE can be to continuously ping the nodes (but it's actually not useful as we already have a segment transfer call)
 *  -> RANKING of rescue nodes
 *
 *  TODO Tree height is also not optimal right now (concerned with bootstrapping & treeHeightOptimization)
 *
 * 1) DONE Generate Packets
 * 2) DONE Schedule them, transfer them to children & rescue nodes
 * 3) DONE Other nodes keep track of incoming packets & transfer them to their children
 *      DONE QUEUE
 * 4) Failure situations
 *      a) Keeping track of ACTIVE NODES in the rescue set
 *      b) RANKING algorithm or heuristics
 *      c) apply it
 *
 *   Node failure -> keep alive messages (any other alternative?)
 *      - Gracefully leaving the overlay [
 *   DONE   sends leave message to both children & parents]
 *   DONE   1) parent look for selecting an alternate for the leaving node
 *          2) children go in rescue mode till the decision is pending
 *          3) children in rescue mode, uses the mesh scheduling in the period [parentLeaveTime till aRescuerFound]
 *              How to keep track of the deadline approaching segments? so that they can be scheduled
 *      - Abrupt failure of the node (
 *          someone need to contact & confirm the failure
 *          rest should be same as graceful leaving)
 *   RPC Timeout implementation
 *
 * 5) Where's the mesh functionality?
 * 6) Collect required statistics
 *      a) Know what parameters are really required or what are the primary factors
 *      b) How to collect them?
 *      c) What kind of reality can be provided in the simulation? (Underlay Configuration is not so good
 *          it doesn't depict the reality. Go with some routers & stuff like that)
 * 7) Emergency video segment SCHEDULING.
 *      DONE Need to allocate identifiers to the segments
 *
 *  // Messages
 *  DONE GetChildrenCall, GetChildrenResponse -> used in getNodesOneUp
 *  DONE VideoSegmentCall -> stream video packet used in transferring the packet
 *  DONE LeaveOverlayCall -> send by leaving node to it's parent & children
 *  DONE LeaveOverlayResponse -> a node using LeaveOverlayCall should wait for this response[TODO MAY BE REMOVE IT]
 *      Directly give the call & wait (to check there's no timeout on the call & retry a few times & simply exit)
 *  DONE NewParentSelectedCall -> gives the description of the newly selected parent
 *  DONE ResponsibilityAsParentCall -> called node is selected as a parent for given set of children
 *
 *  DONE QUEUE or a bounded size buffer to store the packets received
 *
 *  TODO enhancements
 *  1) Bootstrapping need to change a bit
 *  2) Remove a node from bootstrap list once it's full & add it's children in the list
 *      => reduces the number of messages exchanged in joining of a new node
 *
 *  TODO statistics & simulation environment
 *  1) What kind of network topology will work?
 *      One like our institute & other like a general Internet (with nodes behind proxy)
 *  2) What all statistics should be captured?
 *      -> startup time & response time
 *      -> fault tolerance (what happens in the rescue situations)
 *      -> any other characteristics
 *  3) Simulation should be for how many nodes? # of nodes
 *  4) What kind of stream we should be working on? (I'm currently using "strings",
 *          Should I replace it with videoStreams? ->
 *          because segment size may also have some effect on the simulation)
 * */

// To convert between IP addresses (which have bit 24 active), and keys (which don't), we'll need to set or remove this bit.
#define BIGBIT (1 << 24)

/* TODO Utilities
 * Will shift to some other place if required
 * */
template<typename T>
string tToString (T a) {
    std::stringstream ss;
    ss << a;

    return ss.str ();
}


HTopology::~HTopology(){
    // destroy self timer messages
    cancelAndDelete(join_timer);
    cancelAndDelete(packetGenTimer);
}

void HTopology::initialize() {
    // TODO - Generated method body
}

/*void HTopology::handleMessage(cMessage *msg) {
    // TODO - Generated method body
    EV << "got a message in handleMessage: HTopology" << endl;
    delete msg;
}*/

// Called when the module is being initialized
void HTopology::initializeOverlay(int stage) {
    // see BaseApp.cc
   if (stage != MIN_STAGE_OVERLAY) return;

   // get our key from our IP address
   nodeID = thisNode.getIp().get4().getInt() & ~BIGBIT;

   // set the operation mode
   modeOfOperation = GENERAL_MODE;

   //nodeID = thisNode.getIp();
   thisNode.setKey(OverlayKey(nodeID));     // set its key

   // initialize the rest of variables
   bufferMapSize = par("bufferSize");
   maxChildren = par("maxChildren");
   noOfChildren = 0;
   joinRetry = par("joinRetry");
   joinDelay = par("joinDelay");
   packetGenRate = par("packetGenRate");
   isSource = false;

   cache.resize(bufferMapSize);            // Resize the buffer map to fit the given requirement
   cachePointer = 0;
   segmentID = 0;

   EV << "max children is " << maxChildren << endl;

   // add some watches
   WATCH(predecessorNode);
   WATCH(thisNode);
   WATCH(joinRetry);
   WATCH(successorNode);
   WATCH(bootstrapNode);
   // TODO debug the watch of a vector
   //WATCH(cache);
   WATCH(cachePointer);

   // self-messages
   join_timer = new cMessage("join_timer");
   packetGenTimer = NULL;

   EV << thisNode << ": initialized." << std::endl;

   /*rpcTimer = new cMessage("RPC timer: initialized the node");
   scheduleAt(simTime() + 5, rpcTimer);*/
}

// Called to set our own overlay key (optional)
void HTopology::setOwnNodeID() {
    thisNode.setKey(OverlayKey(nodeID));
}

void HTopology::updateTooltip() {
    if (ev.isGUI()) {
        std::stringstream ttString;

        // show our predecessor and successor in tooltip
        ttString << predecessorNode << endl << thisNode << endl
                << successorNode << endl;

        getParentModule()->getParentModule()->getDisplayString().
        setTagArg("tt", 0, ttString.str().c_str());
        getParentModule()->getDisplayString().
        setTagArg("tt", 0, ttString.str().c_str());
        getDisplayString().setTagArg("tt", 0, ttString.str().c_str());

        // parent
        EV << "parent handle in updateToolTip: " << parent.getHandle() << endl;
        showOverlayNeighborArrow(parent.getHandle(), true,
                                         "m=m,50,0,50,0;ls=blue,1");
        // draw an arrow to our current successor
        showOverlayNeighborArrow(successorNode.getHandle(), true,
                                 "m=m,50,0,50,0;ls=red,1");
        // predecessor
        showOverlayNeighborArrow(predecessorNode.getHandle(), false,
                                 "m=m,50,100,50,100;ls=green,1");
    }
}

// change the STATE of this node to state
void HTopology::changeState (int STATE) {
    switch (STATE){
    case INIT:
        state = INIT;
        setOverlayReady(false);

        // initialize predecessor pointer
        parent = grandParent = HNode::unspecifiedNode;
        successorNode = predecessorNode = HNode::unspecifiedNode;

        updateTooltip();

        // debug message
        if (debugOutput) {
            EV << "[Chord::changeState() @ " << thisNode.getIp()
            << " (" << thisNode.getKey().toString(16) << ")]\n"
            << "    Entered INIT stage"
            << endl;
        }

        getParentModule()->getParentModule()->bubble("Enter INIT state.");
        break;

    case JOIN:
        state = JOIN;

        // initiate join process
        cancelEvent(join_timer);
        // workaround: prevent notificationBoard from taking
        // ownership of join_timer message
        take(join_timer);
        scheduleAt(simTime(), join_timer);

        // debug message
        if (debugOutput) {
            EV << "[Chord::changeState() @ " << thisNode.getIp()
            << " (" << thisNode.getKey().toString(16) << ")]\n"
            << "    Entered JOIN stage"
            << endl;
        }

        // find a new bootstrap node and enroll to the bootstrap list
        bootstrapNode = bootstrapList->getBootstrapNode(0);
        //setOverlayReady(true);      // set myself ready

        EV << "bootstrap is " << bootstrapNode << endl ;
        // is this the first node?
        if (bootstrapNode.isUnspecified()) {
            // Initialize this Node as the source node
            EV << "Source node started the overlay." << endl;
            //assert(predecessorNode.isUnspecified());
            isSource = true;

            // Start the packet generation module
            packetGenTimer = new cMessage("Packet Generation Timer");
            schedulePacketGeneration();
        } else {
            // TODO remove the call to getBootstrapNode
            // EV <<"will remove this node from bootstrapping list" << endl;
            //bootstrapList->removeBootstrapNode(thisNode, overlayId);
            EV << thisNode << ": is going to join the overlay rooted at" << bootstrapNode << endl;
            HJoinCall *msg = new HJoinCall();
            msg->setBitLength(JOINCALL_L (msg));

            // send it to the destination
            // TODO Not sure if this is the correct way to do it
            sendRouteRpcCall (OVERLAY_COMP, bootstrapNode, msg);
        }

        changeState(READY);
        updateTooltip();
        getParentModule()->getParentModule()->bubble("Enter JOIN state.");
        break;

    case READY:
        state = READY;
        setOverlayReady(true);

        if (!bootstrapNode.isUnspecified()) {
            EV <<"will remove this node from bootstrapping list" << endl;
            bootstrapList->removeBootstrapNode(thisNode, overlayId);
        }

        // debug message
        if (debugOutput) {
            EV << "[Chord::changeState() @ " << thisNode.getIp()
            << " (" << thisNode.getKey().toString(16) << ")]\n"
            << "    Entered READY stage"
            << endl;
        }
        getParentModule()->getParentModule()->bubble("Enter READY state.");
        break;
    }
}

void HTopology::handleTimerEvent(cMessage* msg) {
    // catch JOIN timer
    if (msg == join_timer) {
        //handleJoinTimerExpired(msg);
        EV << "join timer was called but not handled" << endl;
        EV << "need to do something about this timer" << endl;
    }
    // Packet Generation Timer
    else if(msg == packetGenTimer) {
        handlePacketGenerationTimer(msg);
    }
    // unknown self message
    else {
        error("HTopology::handleTimerEvent(): received self message of "
              "unknown type!");
    }
}

void HTopology::schedulePacketGeneration () {
    cancelEvent(packetGenTimer);
    take (packetGenTimer);
    scheduleAt(simTime()+packetGenRate, packetGenTimer);
}

void HTopology::sendSegmentToChildren(HVideoSegmentCall *videoCall) {
    // Schedule the transfer to all the children & rescue nodes
    // TODO may be we can use the ideology asked in the proposed algorithm
    // Send to only half of the nodes & then ask them to deliver the packet to their neighbors
    MapIterator it;
    for (it = children.begin(); it != children.end(); ++it)
        sendRouteRpcCall (OVERLAY_COMP, (*it).second.getHandle(), videoCall);
    for (it = rescueChildren.begin(); it != rescueChildren.end(); ++it)
        sendRouteRpcCall (OVERLAY_COMP, (*it).second.getHandle(), videoCall);
}

// store the segment in your cache & distribute
void HTopology::handleVideoSegment (BaseCallMessage *msg) {
    HVideoSegmentCall *mrpc = (HVideoSegmentCall *)msg;

    EV << thisNode << ": received " << mrpc->getSegment()
            << ": from -> " << mrpc->getSrcNode() << endl;

    // TODO check the buffer functionalities
    if (cachePointer == bufferMapSize-1) {
        cachePointer=0;    // set pointer to zeroth location
        EV << thisNode
                << ": cacheFull and setting the cachePointer back to 0" << endl;
    }
    cache[cachePointer].videoSegment = mrpc->getSegment();
    cache[cachePointer].segmentID = mrpc->getSegmentID();
    cachePointer++;

    sendSegmentToChildren(mrpc);
    delete msg;
}


// At source node:- packet is generated & sent to the children

void HTopology::handlePacketGenerationTimer(cMessage* msg) {
    schedulePacketGeneration();     // Schedule the run again

    if (state != READY) {
        EV << "Packet generation message called w/o node being in READY state" << endl;
        return;     // If not ready, then it cann't be processed
    }

    // Generate a new message & deliver it to all the nodes
    string pkt = "message-" + tToString(intuniform(0, INT_MAX));

    HVideoSegmentCall *videoCall = new HVideoSegmentCall();
    videoCall->setSegment(pkt.c_str());
    videoCall->setSegmentID(segmentID++);
    videoCall->setBitLength(HVIDEOSEGMENTCALL_L(videoCall)); // TODO BUG CAN BE IN THIS setBitLength thing, please verify it properly

    EV << thisNode << ":Generated message is: " << pkt ;
    sendSegmentToChildren(videoCall);
}


void HTopology::handleJoinTimerExpired(cMessage* msg) {
    // only process timer, if node is not joined yet
    if (state == READY)
        return;

    // enter state JOIN
    if (state != JOIN)
        changeState(JOIN);

    // change bootstrap node from time to time
    joinRetry--;
    if (joinRetry == 0) {
        joinRetry = par("joinRetry");
        changeState(JOIN);
        return;
    }

    // call JOIN RPC
    HJoinCall* call = new HJoinCall("JoinCall");
    call->setBitLength(JOINCALL_L(call));

    RoutingType routingType = (defaultRoutingType == FULL_RECURSIVE_ROUTING ||
                               defaultRoutingType == RECURSIVE_SOURCE_ROUTING) ?
                              SEMI_RECURSIVE_ROUTING : defaultRoutingType;

    sendRouteRpcCall(OVERLAY_COMP, bootstrapNode, thisNode.getKey(),
                     call, NULL, routingType, joinDelay);

    // schedule next join process in the case this one fails
    cancelEvent(join_timer);
    scheduleAt(simTime() + joinDelay, msg);
}

// Called when the module is ready to join the overlay
void HTopology::joinOverlay() {
    if (state == READY) {
        // If already joined the overlay, don't proceed
        // TODO need to debug the spurious joinOverlay() call
        return;
    }

    // tell the simulator that we're ready
    EV << "joinOverlay is called: " << endl ;
    changeState(INIT);
    changeState(JOIN);
}

// Called when the module is about to be destroyed
void HTopology::finishOverlay() {
    // TODO if parent is set, use HLeaveCall
    if (!parent.isUnspecified()) {
        HLeaveOverlayCall *leaveCall = new HLeaveOverlayCall();
        leaveCall->setBitLength(HLEAVEOVERLAYCALL_L(leaveCall));

        // Notifications
        // to parent
        sendRouteRpcCall(OVERLAY_COMP, parent.getHandle(), leaveCall);

        // to children
        for (MapIterator it=children.begin(); it!=children.end(); ++it) {
            sendRouteRpcCall(OVERLAY_COMP, (*it).second.getHandle(), leaveCall);
        }
        // TODO ideally you should wait till you receive response from your parent
    }

    bootstrapList->removeBootstrapNode(thisNode);
    // remove this node from the overlay
    setOverlayReady(false);

    // save the statistics (see BaseApp)
    // globalStatistics->addStdDev("MyOverlay: Dropped packets", numDropped);
    EV << thisNode << ": Leaving the overlay." << std::endl;
}

// Return whether we know if the given node is responsible for the key
bool HTopology::isSiblingFor(const NodeHandle& node,
                             const OverlayKey& key,
                             int numSiblings,
                             bool* err) {
    // is it our node and our key?
    if (node == thisNode && key == thisNode.getKey()) {
        return true;
    }
    // we don't know otherwise
    return false;
}

// Return the next step for the routing of the given message
NodeVector *HTopology::findNode(const OverlayKey& key,
                                int numRedundantNodes,
                                int numSiblings,
                                BaseOverlayMessage* msg) {
    NodeVector* nextHops;

   nextHops = new NodeVector(1);

   // are we responsible? next step is this node
   if (key == thisNode.getKey()) {
       nextHops->add(thisNode);
   }
   // is the key behind us? next step is the previous node
   else if (key < thisNode.getKey()) {
       //nextHops->add(prevNode);
       EV << "prevNode" << std::endl;
   }
   // otherwise, the next step is the next node
   else {
    //   nextHops->add(nextNode);
       EV << "nextNode" << std::endl;
   }
   return nextHops;
}

// Return the max amount of siblings that can be queried about
int HTopology::getMaxNumSiblings() { return 1; }

// Return the max amount of redundant that can be queried about
int HTopology::getMaxNumRedundantNodes() { return 1; }

NodeHandle HTopology::getNodeHandle(MapIterator iter, MapIterator end) {
    if (iter == end) return NodeHandle::UNSPECIFIED_NODE;

    HNode node = (*iter).second;
    return node.getHandle();
}


// NodesOneUp
// respond to the getChildren call
void HTopology::sendChildren (BaseCallMessage *msg) {
    HGetChildrenCall *mrpc = (HGetChildrenCall *) msg;
    HGetChildrenResponse *rrpc =  new HGetChildrenResponse();

    rrpc->setChildrenArraySize(children.size());
    rrpc->setBitLength(HGETCHILDRENRESPONSE_L(rrpc));
    int k=0;
    for (MapIterator it=children.begin(); it!=children.end(); ++it, k++) {
        rrpc->setChildren(k, (*it).second.getHandle());
    }

    sendRpcResponse(mrpc, rrpc);
}

// use the ancestors array to figure out these nodes
void HTopology::initializeNodesOneUp () {
    if (grandParent.isUnspecified()) return;

    // Need to call the last ancestor & get it's children
    // exclude our parent & go ahead with with other nodes
    HGetChildrenCall *msg = new HGetChildrenCall();
    msg->setBitLength(HGETCHILDRENCALL_L (msg));

    // send it to the destination
    sendRouteRpcCall (OVERLAY_COMP, grandParent.getHandle(), msg);
}

// fills up the nodes one up field in the overlay
void HTopology::setNodesOneUp (BaseResponseMessage* msg) {
    HGetChildrenResponse* mrpc = (HGetChildrenResponse*)msg;          // get Response message

    // just remove our parent from this list & add them to the nodesOneUp
    int noOfChildren = mrpc->getChildrenArraySize();
    for (int i=0; i<noOfChildren; ++i) {
        NodeHandle node = mrpc->getChildren(i);

        // ignore our parent
        if (node == parent.getHandle()) {
            EV << "Got our parent in the list: atleast proves this list to be legitimate :P";
            continue;
        }

        HNode hnode;
        hnode.setHandle(node);
        nodesOneUp[node.getKey()] = hnode;
    }
}

void HTopology::handleJoinCall (BaseCallMessage *msg) {
    HJoinCall *mrpc = (HJoinCall*) msg;
    HJoinResponse *rrpc = new HJoinResponse ();
    EV << thisNode << ": received Join call from " <<  msg->getSrcNode() << endl;
    if (capacity()>0) {
        EV << thisNode << ": will be adding node: " << msg->getSrcNode() << ": as child" << endl;
        noOfChildren++;

        // we assume that node is a new comer => everything empty
        HNode child;
        child.setHandle(msg->getSrcNode());

        // there shouldn't already be any node with this key
        assert(children.find(child.getHandle().getKey()) == children.end());
        children[child.getHandle().getKey()] = child;

        MapIterator it = children.find(child.getHandle().getKey());

        // TODO Do we need to tell our parent, regarding we adopting a new child?

        // What is required by a new node?
        // Children & rescueChildren will be empty.
        // TODO nodesOneUp?? (Let the child figure out this parameter)
        // ancestors = thisNode->ancestors + thisNode
        // set the ancestors array
        if (!parent.isUnspecified())
            rrpc->setAncestorsArraySize(ancestors.size() + 1);  // parent of this node will be added as well
        else
            rrpc->setAncestorsArraySize(ancestors.size());      // this node has no parent (it's the source node)

        int k=0;
        for (MapIterator it=ancestors.begin(); it!=ancestors.end(); ++it, ++k) {
            HNode node = (*it).second;
            rrpc->setAncestors(k, node.getHandle());
        }

        if (!parent.isUnspecified()) rrpc->setAncestors (k, parent.getHandle());

        rrpc->setSuccessorNode(getNodeHandle(it++, children.end()));
        rrpc->setPredecessorNode(getNodeHandle(it--, children.end()));
        rrpc->setJoined(true);
    } else {
        // TODO may be check if anyone of them can support the children
        // Try to keep the height of the tree as low as possible
        EV << "redirecting to one of my children: " << endl;

        size_t size = children.size();
        int redirection = intuniform(0, size-1);
        MapIterator iter = children.begin();
        for (int i=0; i<redirection; ++i, iter++);

        HNode redirectionChild = (*iter).second;
        rrpc->setSuccessorNode(redirectionChild.getHandle());
        rrpc->setJoined(false);
    }

    rrpc->setBitLength(HJOINRESPONSE_L(rrpc));
    sendRpcResponse(mrpc, rrpc);
}

void HTopology::handleLeaveCall (BaseCallMessage *msg) {
    /*
     * If you are the parent of the caller -> find a replacement of the node
     * Else
     *  you go in rescue mode
     * */

    HLeaveOverlayCall *mrpc = (HLeaveOverlayCall *)msg;

    if (parent.getHandle() == mrpc->getSrcNode()) {
        EV << " My parent is leaving the overlay " << endl;

        /*
         * 1) Go in rescue mode
         * 2) Schedule your deadlines as per the ranked rescue set
         *      Until your grandparent tells you the replacement
         * */
        modeOfOperation = RESCUE_MODE;
    }
    else {
        assert(children.find(mrpc->getSrcNode().getKey()) != children.end());
        /*
         * 1) Find replacement for the src of the message
         * 2) Notify the replacement to all the children after the decision is taken
         *
         * TODO who should take this decision? parent of the node or the children?
         * children can also do it via LEADER SELECTION, right?
         * */
        // Let the parent handle this situation, for now

        HNodeReplacement nreplacement;
        nreplacement.node = mrpc->getSrcNode();
        nreplacement.mrpc = mrpc;
        leaveRequests[mrpc->getSrcNode().getKey()] = nreplacement;

        getParametersForSelectionAlgo(mrpc->getSrcNode().getKey());

        // TODO do we need to setup any other parameter?
    }
}

void HTopology::handleNewParentSelectedCall (BaseCallMessage *msg) {
    HNewParentSelectedCall *mrpc = (HNewParentSelectedCall*)msg;

    // Ideally this node should also be acting in rescue mode
    assert (modeOfOperation == RESCUE_MODE);
    assert (!(mrpc->getParent().isUnspecified()));

    parent.setHandle(mrpc->getParent());
    // TODO get the children of our parent
    modeOfOperation = GENERAL_MODE;
}

void HTopology::handleResponsibilityAsParentCall (BaseCallMessage *msg) {
    HResponsibilityAsParentCall *mrpc = (HResponsibilityAsParentCall *)msg;

    // Ideally this node should also be acting in rescue mode
    // TODO may change when we change repair strategy
    assert (modeOfOperation == RESCUE_MODE);
    assert (!(mrpc->getParent().isUnspecified()));

    parent.setHandle(mrpc->getParent());
    modeOfOperation = GENERAL_MODE;

    // add the children vector into our children set
    for (size_t i=0; i<mrpc->getChildrenArraySize(); ++i, ++noOfChildren) {
        HNode node;
        node.setHandle(mrpc->getChildren(i));

        // TODO ask them about their children
        children[node.getHandle().getKey()] = node;
    }
    // TODO do we update our parent about our new children? I Think NO. He already know about them
}


// RPC
bool HTopology::handleRpcCall(BaseCallMessage *msg) {
    // There are many macros to simplify the handling of RPCs. The full list is in <OverSim>/src/common/RpcMacros.h.
    // start a switch
    RPC_SWITCH_START(msg);

    // enters the following block if the message is of type HCapacityCall (note the shortened parameter!)
    RPC_ON_CALL(HCapacity) {
        HCapacityCall *mrpc = (HCapacityCall*)msg;          // get Call message
        HCapacityResponse *rrpc = new HCapacityResponse();  // create response
        rrpc->setRespondingNode(thisNode);

        // TODO the capacity can be decided dynamically, but for now lets use the static version
        rrpc->setCapacity(capacity());      // set the capacity left at this node
        rrpc->setBitLength(HCAPACITYRESPONSE_L(rrpc));

        // now send the response. sendRpcResponse can automatically tell where to send it to.
        // note that sendRpcResponse will delete mrpc (aka msg)!
        sendRpcResponse(mrpc, rrpc);

        RPC_HANDLED = true;  // set to true, since we did handle this RPC (default is false)
        break;
    }

    RPC_ON_CALL(HSelectParent) {
        //HSelectParentCall *mrpc = (HSelectParentCall)msg;

        // TODO Do check if the request is not a fake/malicious one
        RPC_HANDLED = true;
        break;
    }

    RPC_ON_CALL(HVideoSegment) {
        handleVideoSegment(msg);
        RPC_HANDLED = true;
        break;
    }

    RPC_ON_CALL(HLeaveOverlay) {
       handleLeaveCall(msg);
       RPC_HANDLED = true;
       break;
    }

    RPC_ON_CALL(HResponsibilityAsParent) {
       handleResponsibilityAsParentCall(msg);
       RPC_HANDLED = true;
       break;
    }

    RPC_ON_CALL(HNewParentSelected) {
       handleNewParentSelectedCall(msg);
       RPC_HANDLED = true;
       break;
    }

    RPC_ON_CALL(HGetChildren) {
        sendChildren (msg);
        RPC_HANDLED = true;
        break;
    }

    RPC_ON_CALL(HJoin) {
        handleJoinCall (msg);
        RPC_HANDLED = true;
        break;
    }

    // end the switch
    RPC_SWITCH_END();

    // return whether we handled the message or not.
    // don't delete unhandled messages!
    return RPC_HANDLED;
}

// Called when an RPC we sent has timed out.
// Don't delete msg here!
// TODO
void HTopology::handleRpcTimeout(BaseCallMessage* msg,
                                 const TransportAddress& dest,
                                 cPolymorphic* context, int rpcId,
                                 const OverlayKey&) {
    // Same macros as in handleRpc
/*
    // start a switch
    RPC_SWITCH_START(msg);
        // enters the following block if the message is of type MyNeighborCall (note the shortened parameter!)
        RPC_ON_CALL(MyNeighbor) {
            MyNeighborCall *mrpc = (MyNeighborCall*)msg;          // get Call message
            callbackTimeout(mrpc->getDestinationKey());           // call our interface function
        }
    // end the switch
    RPC_SWITCH_END(); */
}

void HTopology::handleCapacityResponse (BaseResponseMessage *msg) {
    HCapacityResponse *mrpc = (HCapacityResponse*)msg;          // get Response message
    //if (queryNodesSelectionAlgo.find(mrpc->getSrcNode().getKey()) != queryNodesSelectionAlgo.end())
    // set the capacity for the key

    OverlayKey key = mrpc->getParentNode().getKey();
    if (leaveRequests.find(key) == leaveRequests.end()) {
        // TODO
        EV << "some malicious activity going around" << endl;
        return;
    }

    std::map<OverlayKey, int>& queryNodesSelectionAlgo = leaveRequests[key].queryNodesSelectionAlgo;
    size_t responseRequired = leaveRequests[key].responseRequired;

    queryNodesSelectionAlgo[key] = mrpc->getCapacity();
    if (queryNodesSelectionAlgo.size() == responseRequired) {
        goAheadWithRestSelectionProcess (leaveRequests[key].node.getKey());
    }
}

// Called when we receive an RPC response from another node.
// Don't delete msg here!
void HTopology::handleRpcResponse(BaseResponseMessage* msg,
                                  cPolymorphic* context,
                                  int rpcId,
                                  simtime_t rtt) {
    // The macros are here similar. Just use RPC_ON_RESPONSE instead of RPC_ON_CALL.

    // start a switch
    RPC_SWITCH_START(msg);
        // enters the following block if the message is of type MyNeighborResponse (note the shortened parameter!)
        RPC_ON_RESPONSE(HCapacity) {
            handleCapacityResponse(msg);
        }

        RPC_ON_RESPONSE(HSelectParent) {
            // TODO, will there really be any response message of this sort?
        }

        RPC_ON_RESPONSE(HGetChildren) {
            setNodesOneUp(msg);
        }

        RPC_ON_RESPONSE(HJoin) {
            HJoinResponse* mrpc = (HJoinResponse*)msg;          // get Response message
            if (mrpc->getJoined() == true) {
                EV << "We got a response from " << mrpc->getSrcNode() << endl;
                parent.setHandle(mrpc->getSrcNode());

                if (mrpc->getAncestorsArraySize() > 0)
                    grandParent.setHandle(mrpc->getAncestors(mrpc->getAncestorsArraySize()-1));

                successorNode.setHandle(mrpc->getSuccessorNode());
                predecessorNode.setHandle(mrpc->getPredecessorNode());

                for (int i=0; i<mrpc->getAncestorsArraySize(); ++i) {
                    NodeHandle node = mrpc->getAncestors(i);
                    HNode hnode;
                    hnode.setHandle(node);
                    ancestors[node.getKey()] = hnode;
                }

                EV << "joined overlay :" ;
                updateTooltip();
            } else {
                EV << thisNode << ": is going to join the overlay rooted at" << mrpc->getSuccessorNode() << endl;
                HJoinCall *mcall = new HJoinCall();
                mcall->setBitLength(JOINCALL_L (mcall));

                // go on & call the given node
                if (mrpc->getSuccessorNode().isUnspecified()) {
                    EV << "Incorrect response->" ;
                    EV << "NEED HELP" << endl;
                } else {
                    // go ahead & add yourself to the child
                    EV << "joining at " << mrpc->getSuccessorNode() <<  endl;
                    sendRouteRpcCall (OVERLAY_COMP, mrpc->getSuccessorNode(), mcall);
                }
            }
            // TODO when to delete which message?
            // there'll be lots of memory leaks :D
        }

    // end the switch
    RPC_SWITCH_END();
}

// AddOns

// select replacement for node
// 1) Setting Up the required Parameters for the procedure
void HTopology::getParametersForSelectionAlgo (const OverlayKey& key) {
    if (leaveRequests.find(key) != leaveRequests.end()) {
        EV << "some spurious call to getParametersForSelectionAlgo" << endl;
        return;
    }

    if (children.find(key) == children.end()) {
        EV << "replacement of the given node is not a responsibility of this node" << endl;
        return;
    }

    NodeVector nodeChildren = children[key].getNodeVector();

    leaveRequests[key].queryNodesSelectionAlgo.clear();        // clear this variable for storing the recent values
    leaveRequests[key].responseRequired=nodeChildren.size();   // Response required
    for (NodeVector::iterator it=nodeChildren.begin(); it!=nodeChildren.end(); ++it) {
        // Prepare the capacity message
        HCapacityCall *msg = new HCapacityCall();
        NodeHandle node = *it;
        msg->setDestinationKey(node.getKey());
        msg->setBitLength(HCAPACITYCALL_L(msg));

        // send it to the destination
        // TODO Not sure if this is the correct way to do it
        sendRouteRpcCall (OVERLAY_COMP, node.getKey(), msg);
    }
}

// 2) Main procedure for deciding the replacement for keyParent
void HTopology::goAheadWithRestSelectionProcess(const OverlayKey& key) {
    if (leaveRequests.find(key) != leaveRequests.end()) {
        EV << "some spurious call to goAheadWithRestSelectionAlgo" << endl;
        return;
    }

    if (children.find(key) == children.end()) {
        EV << "replacement of the given node is not a responsibility of this node " << endl;
        return;
    }

    int noOfChildrenToAdd = children[key].getNodeVector().size();
    bool replacementDone = false;
    std::map<OverlayKey, int>& queryNodesSelectionAlgo = leaveRequests[key].queryNodesSelectionAlgo;
    std::map<OverlayKey, int>::iterator it=leaveRequests[key].queryNodesSelectionAlgo.begin();

    for (; it!=queryNodesSelectionAlgo.end(); ++it) {
        // What else is to be done?
        // Modify the existing next & prev pointers for the replacement's sibling
        // Modify the pointers for the siblings in the current overlay
        // And add nodeChildren as parent to the replacement node
        int capacity = (*it).second;
        if (capacity >= noOfChildrenToAdd) {
            replacementDone = true;
            break;
        }
    }

    if (replacementDone) {
        // 1) Notify the node about the result
        // Send the HLeaveOverlayResponse to the node, whose replacement has been found
        HLeaveOverlayResponse *rrpc = new HLeaveOverlayResponse();
        rrpc->setPermissionGranted(true);
        rrpc->setBitLength(HLEAVEOVERLAYRESPONSE_L(rrpc));
        sendRpcResponse(leaveRequests[key].mrpc, rrpc);


        // 2) Replace the child with this newly selected parent (children list)
        NodeVector newChildren = children[key].getNodeVector();

        NodeVector::iterator pos = newChildren.find((*it).first);
        NodeHandle newHandle = *pos;

        if (pos != newChildren.end()) newChildren.erase(pos);
        else {
            EV << "Is this even possible that the replacement node is not in the children set?" << endl;
        }
        HNode newNode = children[key];
        NodeHandle oldNode = newNode.getHandle();
        children.erase(key);
        newNode.setHandle(newHandle);
        newNode.setNodeVector(newChildren);
        children[newHandle.getKey()] = newNode;


        // 3) Inform the newParent about its children
        HResponsibilityAsParentCall *mrpc = new HResponsibilityAsParentCall();
        mrpc->setChildrenArraySize(newChildren.size());
        for (size_t i=0; i<newChildren.size(); ++i) {
            mrpc->setChildren(i, newChildren[i]);
        }
        mrpc->setParent(thisNode);
        mrpc->setBitLength(HRESPONSIBILITYASPARENTCALL_L(mrpc));
        sendRouteRpcCall(OVERLAY_COMP, oldNode, mrpc);

        // 4) Inform children about the new parent
        // TODO what about the successor & predecessor
        HNewParentSelectedCall *msg = new HNewParentSelectedCall();
        msg->setParent(newNode.getHandle());
        msg->setBitLength(HNEWPARENTSELECTEDCALL_L (msg));

        for (int i=0; i<newChildren.size(); ++i)
            sendRouteRpcCall(OVERLAY_COMP, newChildren[i], msg);
        // END BOOK-KEEPING
    } else {
        // Couldn't select a parent for the children,
        // WHAT's THE FALL BACK OPTION?
        EV << "Couldn't select a new node" << endl;
    }

    leaveRequests.erase (key);
    // 1) Get the node's children from the handle
    // 2) Job is to pick a node from this list & make it the new parent & let this propagate till the last level
    //  or Let's pick a node at last level & make it the new parent
    //   We can also look at some of the characteristics
    //          It should be having enough capacity to support all the children in the list
    //              (probably second approach will always work).
    //          Any other transfer characteristics?

    // Once done with "deciding the new parent", inform the children about their new parent
    // Also inform the new parent for the same
}

// generate transfer characteristics & rank them
void HTopology::rankRescueNodes () {
    // TODO
    // Need to decide on some transfer characteristics in order to rank the nodes
    // same can be used for selection of parent in this kind of scenario
    // TODO
}

// choose a rescue parent for yourself
bool HTopology::selectRescueParent () {
    // This should be quite an easy job once we are done with "Ranking the rescue nodes"
    // Just go one by one to all the nodes in the rescue list in descending order of their
    // ranks & ask if they can serve as rescuers, if yes go ahead and change the mode to RESCUE
    // ultimately we should be allotted a rescue position (at SOURCE NODE)

    // send the node a rescue signal

    // during the rescue situation we should be using our network nodes for transferring the
    // deadline segments, may be use the transfer algorithm from Anysee.
}

// add "node" as a rescue child
bool HTopology::addAsRescueChild (const NodeHandle& node) {
    // send the rescue signal to the node
    // don't duplicate the work of being in RESCUE situation,
    // we can save the bandwidth via using just one call to a rescuer
}

// remove this node from rescue children list
bool HTopology::removeRescueChild (const NodeHandle& node) {
    // just signal the node to remove myself from being a rescue to it
}

// look for alternatives on deadline approaching segments
void HTopology::scheduleDeadlineSegments () {
    // TODO
}

// Advance Features
void HTopology::optimizeTree () {
    // TODO
}

void HTopology::calculateResourceAllocationPolicy () {
    // TODO
}
