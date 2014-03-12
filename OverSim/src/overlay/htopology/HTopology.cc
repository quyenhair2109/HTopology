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
 * TODO Go through all the TODO's & resolve them if possible
 * TODO Sit someday & fix the message lengths [we've updated the message structures but the lengths remain the same]
 * TODO A node may not have the RescueParametersTimer scheduled [because it has no ancestor (we don't count
 *      the parent here) && hence no nodesOneUp]. But if sometime later it populates these parameters,
 *      then we'll have to schedule the timer [REMEMBER THIS THING AT RE-JOINING the NODE AT SOME OTHER PARENT]
 *
 * DONE
 * MAJOR CONCERN NOW
 *  -> SCHEDULING (in rescue mode over a set of nodes)
 *      Can  be taken care of, if only RANKING works fine
 * DONE => just schedule the segments in one-by-one in the increasing order of timeRemainingInDeadline
 *          in the ranked rescue set
 *  -> ACTIVENESS of Nodes (liveness of nodes & keep alive messages)
 * DONE Activeness (check in case of abrupt node failures)
 *      - DONE can be associated with a timeout to the videoSegmentCall (i.e. it doesn't reach the node)
 *      - ALTERNATE can be to continuously ping the nodes (but it's actually not useful as we already have a segment transfer call)
 *  -> RANKING of rescue nodes
 *      Let's enumerate all the parameters affecting the ranking (mainly answering the questions
 *          "Am I responsible for node's ability to serve as rescue node?")
 *
 *          -> RTT
 *          -> RescueCapacityLeft
 *          -> EffectiveBandwidth
 *          ->
 *
 *          Periodically gather information about these parameters from the potentially rescue nodes
 *
 *  TODO Improve the ranking factors
 *          Ranking (for the time being, assume to be a linear function of these parameters with some weights
 *              which need to derived (experimentally or heuristically))
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
 *   DONE   2) children go in rescue mode till the decision is pending
 *          3) children in rescue mode, uses the mesh scheduling in the period [parentLeaveTime till aRescuerFound]
 *              How to keep track of the deadline approaching segments? so that they can be scheduled
 *
 *   OUR GRANDPARENT CAN ALWAYS BE OUR RESCUE NODE [WILL REMOVE THE OVERHEAD OF SELECTING THE RESCUE PARENT]
 *      - Abrupt failure of the node (
 *          DONE someone need to contact & confirm the failure
 *          DONE rest should be same as graceful leaving)
 *   RPC Timeout implementation
 *      DONE for node failure in case of videoSegment Transfer
 *
 * 5) Where's the mesh functionality?
 * 6) Collect required statistics
 *      a) Know what parameters are really required or what are the primary factors
 *      b) How to collect them?
 *      c) What kind of reality can be provided in the simulation? (Underlay Configuration is not so good
 *          it doesn't depict the reality. Go with some routers & stuff like that)
 *
 *      [ANSWER TO SIMULATION EVENT RELATED QUESTIONS]
 *          No. of nodes -> as many as possible
 *          Underlay configuration -> we are only worried about the overlay configuration
 *          Parameters -> checkout simulation events in papers
 *          Stream -> try to use the video stream (but only after this whole thing works with strings)
 *
 * 7) DONE Emergency video segment SCHEDULING.
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
 *  DONE ScheduleSegmentsCall, ScheduleSegmentsResponse -> asking to send some of the segments within [SegmentID, SegmmentID + count]
 *  DONE SwitchToRescueModeCall  -> ask these nodes to switch to rescue modes as their parent node failed
 *  HGetParametersCall, HGetParametersResponse -> ask the ranking parameters to the node
 *      DONE Start a timer, so that we can keep fresh data in here
 *
 *  cache can be managed by buffer depicted in Anysee or STL-<map>
 *  DONE FIXED SIZE SEGMENTS
 *  DONE QUEUE or a bounded size buffer to store the packets received
 *
 *  TODO TIMERS
 *     DONE  1) Generation of packets
 *      2) Deadline of segments as per given packet generation rate
 *          [this will always be known or remain constant throughout the
 *          streaming life cycle]
 *      3) Calculating the parameters [ranking parameters]
 *     DONE Basic Functionality
 *          TODO
 *               - should be called once every (2*MAX-RTT or 4*MAX-RTT), otherwise there'll be lots of overhead
 *               - should be carefully set to a bit higher value than the once currently set
 *      4) Do we need to refresh data in our children parameters, with the help of timers?
 *
 *  TODO enhancements
 *  1) Bootstrapping need to change a bit
 *  2) Remove a node from bootstrap list once it's full & add it's children in the list
 *      => reduces the number of messages exchanged in joining of a new node
 *
 *  TODO statistics & simulation environment
 *    DONE startup time -> node's 1st request to join && the acceptance response
 *    DONE startup packet -> node joining time && 1st packet received
 *    DONE transfer delay -> packet issuance time && receiving simulation time TODO do we need to synchronize all the clocks
 *         packet loss [#of packets reaching after the deadline is over, or not at all reaching]
 *         How does a node know, when a segment is reaching the deadline?
 *             - May be use the SYNC signal sent by the root to all the nodes[so that they can figure out their status in the stream]
 *    DONE  total number of messages sent [divide into all the types we've defined in HMessage.msg file]
 *
 *      TODO parameter estimation shouldn't be done by the root node, anyways there are bugs associated with the current timer
 *      number of times parameterEstimation was done.
 *      ranks calculated [average rank available at a node, maximum, minimum & median]
 *      load at a node -> #ofChildren && #ofRescueChildren
 *    DONE #ofPacketsGenerated
 *    DONE[transfer delay handles this]  max reaching time of this packet to any node
 *      max height of the tree
 *
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

bool compareRescueNodes (const RescueNode& L, const RescueNode& R) { return L.getRank() > R.getRank(); }


HTopology::~HTopology(){
    // destroy self timer messages
    cancelAndDelete(join_timer);
    cancelAndDelete(packetGenTimer);
    cancelAndDelete(rescueParametersTimer);
}

void HTopology::initialize() {
    // TODO - Generated method body
}

/*void HTopology::handleMessage(cMessage *msg) {
    // TODO - Generated method body
    EV << "got a message in handleMessage: HTopology" << endl;
    delete msg;
}*/

void HTopology::initializeStats () {
    // #ofMessages sent & recvd
    memset(numSentMessages, 0, sizeof numSentMessages);
    memset(numRecvMessages, 0, sizeof numRecvMessages);

    notReceivedPacket=true;
    parameterEstimationRounds=numPackets=0;
}

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
   maxRescueChildren = par("maxRescueChildren");
   bandwidth = par("bandwidth");
   noOfChildren = 0;
   joinRetry = par("joinRetry");
   joinDelay = par("joinDelay");
   packetGenRate = par("packetGenRate");

   // TODO this should be estimated in varying period of time [instead of estimating regularly]
   // Initially it should be done at a bit higher rate, then frequency should reduce gradually
   // Can you check what's the use of Jain's Parameter? [We studied in Networks course, if that can be applied here]
   rescueParameterEstimationRate = par("rescueParameterEstimationRate");
   isSource = false;

   initializedRescueRanks = false;

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
   rescueParametersTimer = NULL;

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
            scheduleTimer(packetGenTimer, packetGenRate);
        } else {
            joinRequestTime = simTime();            // FILL IN THE JOIN REQUEST TIME

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

        // Parameter estimation will not be done by the root node
        if (!isSource) {
            if (rescueParametersTimer == NULL)
                rescueParametersTimer = new cMessage ("Rescue_Parameters_Timer");
            scheduleTimer(rescueParametersTimer, rescueParameterEstimationRate);
        }

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

        // Start the rescueParameter estimation timer
        rescueParametersTimer = new cMessage("Rescue Parameters Estimation Timer");
        scheduleTimer(rescueParametersTimer, rescueParameterEstimationRate);
        break;
    }
}

void HTopology::handleRescueParametersEstimationTimer (cMessage *msg) {
    parameterEstimationRounds++;

    /* Reset the parameters response variables
     * TODO associate a number denoting the parameter estimation round
     *      [so that we don't have collision between two invocations]
     *  DONE add a timeout associated with this message, so that we can debug the current timers problem
     *       - BTW there was not such problem, instead the nodes didn't have the ancestors & nodesOneUp :)
     */
    parametersResponseReceived=parametersResponseRequired=0;
    for (KeyToRescueNodeMap::iterator it=ancestors.begin();
            it!=ancestors.end(); ++it, parametersResponseRequired++) {
        if ((*it).first == thisNode.getKey()) {
            EV << "We were sending self message via ancestors" << endl;
            continue;
        }
        HGetParametersCall *parametersCall = new HGetParametersCall();
        sendRouteRpcCall (OVERLAY_COMP, (*it).second.getHandle(), (*it).first, parametersCall);
    }

    for (KeyToRescueNodeMap::iterator it=nodesOneUp.begin();
            it!=nodesOneUp.end(); ++it, parametersResponseRequired++) {
        if ((*it).first == thisNode.getKey()) {
            EV << "We were sending self message via nodesOneUp" << endl;
            continue;
        }
        HGetParametersCall *parametersCall = new HGetParametersCall();
        sendRouteRpcCall (OVERLAY_COMP, (*it).second.getHandle(), (*it).first, parametersCall);
    }
    // TODO should we reschedule the call here? I DON'T THINK SO
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
        EV << "msg owner is " << msg->getOwner()->getClassName() << endl;
        EV << "packet generation timer" << endl;
        handlePacketGenerationTimer(msg);
    }
    // Gather the rescue parameters
    else if(msg == rescueParametersTimer) {
        EV << "parameters estimation timer" << endl;
        handleRescueParametersEstimationTimer (msg);
    }
    // unknown self message
    else {
        EV << "Unknown Message is: " << msg->getClassName() << endl;
        //error("HTopology::handleTimerEvent(): received self message of "
        //      "unknown type!");
    }
}

void HTopology::scheduleTimer(cMessage* timer, double rate) {
    cancelEvent(timer);
    take (timer);
    scheduleAt(simTime()+rate, timer);
}

void HTopology::sendSegmentToChildren(HVideoSegment segment) {
    // Schedule the transfer to all the children & rescue nodes
    // TODO may be we can use the ideology asked in the proposed algorithm
    // Send to only half of the nodes & then ask them to deliver the packet to their neighbors
    EV << "Called sendSegmenttochildren" << endl;
    MapIterator it; int i=0;
    for (it = children.begin(); it != children.end(); ++it, ++i) {
        EV << "Got it : " << i << endl;

        HVideoSegmentCall *videoCall = new HVideoSegmentCall();
        videoCall->setSegment(segment);
        videoCall->setBitLength(HVIDEOSEGMENTCALL_L(videoCall));

        sendRouteRpcCall (OVERLAY_COMP, (*it).second.getHandle(), (*it).first, videoCall);
    }
    for (it = rescueChildren.begin(); it != rescueChildren.end(); ++it) {
        HVideoSegmentCall *videoCall = new HVideoSegmentCall();
        videoCall->setSegment(segment);
        videoCall->setBitLength(HVIDEOSEGMENTCALL_L(videoCall));

        sendRouteRpcCall (OVERLAY_COMP, (*it).second.getHandle(), (*it).first, videoCall);
    }
}

// store the segment in your cache & distribute
void HTopology::handleVideoSegment (BaseCallMessage *msg) {
    numSentMessages[EVideoSegment]++;
    if (notReceivedPacket) {
        firstPacketRecvingTime = simTime();
        notReceivedPacket = false;
    }

    HVideoSegmentCall *mrpc = (HVideoSegmentCall *)msg;

    EV << thisNode << ": received " << mrpc->getSegment().videoSegment
            << ": from -> " << mrpc->getSrcNode() << endl;

    // TODO transferDelay = (simTime() - mrpc->getSegment().issuanceTime), how do we plot or use this info
    EV << "transfer delay for this packet: " << (simTime() - mrpc->getSegment().issuanceTime) << endl;

    addSegmentToCache(mrpc->getSegment());

    /*MapIterator it=children.begin();
    if (it !=children.end())
        sendRouteRpcCall (OVERLAY_COMP, (*it).second.getHandle(), (*it).first, mrpc);*/
    sendSegmentToChildren(mrpc->getSegment());

    // Send response back to the sending node
    HVideoSegmentResponse *rrpc =  new HVideoSegmentResponse();
    rrpc->setBitLength(HVIDEOSEGMENTRESPONSE_L(rrpc));
    sendRpcResponse(mrpc, rrpc);

    //delete msg;
}


HVideoSegment HTopology::generateVideoSegment () {
    EV << "Generating a new video segment" << endl;
    // Generate a new message & deliver it to all the nodes
    string pkt = "message-" + tToString(intuniform(0, INT_MAX));

    HVideoSegment segment;
    segment.segmentID = segmentID++;
    numPackets++;
    segment.issuanceTime = simTime();
    strncpy (segment.videoSegment, pkt.c_str(), SEGMENT_SIZE-1);
    return segment;
}

// At source node:- packet is generated & sent to the children

void HTopology::handlePacketGenerationTimer(cMessage* msg) {
    // Schedule the run again
    scheduleTimer(packetGenTimer, packetGenRate);

    if (state != READY) {
        EV << "Packet generation message called w/o node being in READY state" << endl;
        return;     // If not ready, then it cann't be processed
    }

    HVideoSegment segment = generateVideoSegment();
    EV << thisNode << ":Generated message is: " << segment.videoSegment;// videoCall->getSegment().videoSegment ;
    sendSegmentToChildren(segment);
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
    if (!parent.isUnspecified()) {
        leaveRequestTime = simTime();
        HLeaveOverlayCall *leaveCall = new HLeaveOverlayCall();
        leaveCall->setBitLength(HLEAVEOVERLAYCALL_L(leaveCall));

        // Notifications
        // to parent
        sendRouteRpcCall(OVERLAY_COMP, parent.getHandle(), leaveCall);

        // to children
        for (MapIterator it=children.begin(); it!=children.end(); ++it) {
            HLeaveOverlayCall *leaveCall = new HLeaveOverlayCall();
            leaveCall->setBitLength(HLEAVEOVERLAYCALL_L(leaveCall));
            sendRouteRpcCall(OVERLAY_COMP, (*it).second.getHandle(), (*it).first, leaveCall);
        }
        // TODO ideally you should wait till you receive response from your parent
    }

    bootstrapList->removeBootstrapNode(thisNode);
    // remove this node from the overlay
    setOverlayReady(false);

    // save the statistics (see BaseApp)
    // globalStatistics->addStdDev("MyOverlay: Dropped packets", numDropped);
    globalStatistics->addStdDev("VideoSegmentSent", numSentMessages[EVideoSegment]);
    globalStatistics->addStdDev("VideoSegmentReceived", numRecvMessages[EVideoSegment]);
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
    numSentMessages[EGetChildren]++;

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
    numRecvMessages[EGetChildren]++;
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

        RescueNode hnode;
        hnode.setHandle(node);
        nodesOneUp[node.getKey()] = hnode;
    }
}

void HTopology::handleJoinCall (BaseCallMessage *msg) {
    numSentMessages[EJoin]++;

    HJoinCall *mrpc = (HJoinCall*) msg;
    HJoinResponse *rrpc = new HJoinResponse ();
    EV << thisNode << ": received Join call from " <<  msg->getSrcNode() << endl;

    // TODO capacity()>0 && modeOfOperation==GENERAL_MODE
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
        // nodesOneUp?? (Let the child figure out this parameter)
        // ancestors = thisNode->ancestors + thisNode
        // set the ancestors array
        if (!parent.isUnspecified())
            rrpc->setAncestorsArraySize(ancestors.size() + 1);  // parent of this node will be added as well
        else
            rrpc->setAncestorsArraySize(ancestors.size());      // this node has no parent (it's the source node)

        int k=0;
        for (RescueMapIterator it=ancestors.begin(); it!=ancestors.end(); ++it, ++k) {
            RescueNode node = (*it).second;
            rrpc->setAncestors(k, node.getHandle());
        }

        if (!parent.isUnspecified()) rrpc->setAncestors (k, parent.getHandle());

        rrpc->setSuccessorNode(getNodeHandle(it++, children.end()));
        rrpc->setPredecessorNode(getNodeHandle(it--, children.end()));
        rrpc->setJoined(true);

        // Remove yourself from the bootstrapping in case your capacity==0
        if (capacity()==0) {
            // remove yourself from the bootstrapping thing
            // && care to check if your children are there in the bootstrapping list
            for (MapIterator it=children.begin(); it !=children.end(); ++it) {
                // Insert our children into the bootstrap list
                bootstrapList->registerBootstrapNode((*it).second.getHandle(), overlayId);
                //bootstrapList->insertBootstrapCandidate((*it).second.getHandle());
            }
            bootstrapList->removeBootstrapNode(thisNode, overlayId);
        }
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

void HTopology::selectReplacement (const NodeHandle& node, HLeaveOverlayCall *mrpc) {
    /*
     * 1) Find replacement for the src of the message
     * 2) Notify the replacement to all the children after the decision is taken
     *
     * TODO who should take this decision? parent of the node or the children?
     * children can also do it via LEADER SELECTION, right?
     * */
    // Let the parent handle this situation, for now

    // TODO mrpc is NULL in handleRpcTimeout event
    EV << "Called the select replacement event, but no functionality yet."
            " We'll look for the functionality after debugging the issue" << endl;

/*    HNodeReplacement nreplacement;
    nreplacement.node = mrpc->getSrcNode();
    nreplacement.mrpc = mrpc;
    leaveRequests[mrpc->getSrcNode().getKey()] = nreplacement;

    getParametersForSelectionAlgo(mrpc->getSrcNode().getKey());*/

    // TODO do we need to setup any other parameter?
}


void HTopology::handleLeaveCall (BaseCallMessage *msg) {
    numSentMessages[ELeaveOverlay]++;
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
        selectReplacement(mrpc->getSrcNode(), mrpc);
    }
}

void HTopology::handleNewParentSelectedCall (BaseCallMessage *msg) {
    numSentMessages[ENewParentSelected]++;

    HNewParentSelectedCall *mrpc = (HNewParentSelectedCall*)msg;

    // Ideally this node should also be acting in rescue mode
    assert (modeOfOperation == RESCUE_MODE);
    assert (!(mrpc->getParent().isUnspecified()));

    parent.setHandle(mrpc->getParent());
    // TODO get the children of our parent
    modeOfOperation = GENERAL_MODE;
    // TODO adjust our successor & predecessor ??
}

void HTopology::handleResponsibilityAsParentCall (BaseCallMessage *msg) {
    numSentMessages[EResponsibilityAsParent]++;

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
void HTopology::handleScheduleSegmentsCall (BaseCallMessage *msg) {
    numSentMessages[EScheduleSegments]++;

    HScheduleSegmentsCall *scheduleCall = (HScheduleSegmentsCall *)msg;
    int startSegmentID = scheduleCall->getStartSegmentID();
    int count = scheduleCall->getCount();

    vector<HVideoSegment> foundSegments;
    // TODO you can maintain the cache in the form of a heap [stl<map> will be a good thing as it stores in increasing order of keys]
    // TODO optimize this thing
    for (int cnt=0; cnt<count; ++cnt) {
        for (size_t i=0; i<cache.size(); ++i) {
            if (cache[i].segmentID == startSegmentID+cnt) {
                foundSegments.push_back(cache[i]);
            }
        }
    }

    // prepare the response
    HScheduleSegmentsResponse *scheduleResponse = new HScheduleSegmentsResponse();
    scheduleResponse->setSegmentsArraySize(foundSegments.size());
    scheduleResponse->setBitLength(HSCHEDULESEGMENTSRESPONSE_L(scheduleResponse));
    for (size_t k=0; k<foundSegments.size(); ++k)
        scheduleResponse->setSegments(k, foundSegments[k]);
    sendRpcResponse(scheduleCall, scheduleResponse);
}

void HTopology::addSegmentToCache (HVideoSegment& videoSegment) {
    // TODO check the buffer functionalities
    if (cachePointer == bufferMapSize-1) {
        cachePointer=0;    // set pointer to zeroth location
        EV << thisNode
                << ": cacheFull and setting the cachePointer back to 0" << endl;
    }

    cache[cachePointer++] = videoSegment;
}

void HTopology::handleScheduleSegmentsResponse (BaseResponseMessage *msg) {
    numRecvMessages[EScheduleSegments]++;

    HScheduleSegmentsResponse *scheduleResponse = (HScheduleSegmentsResponse *)msg;
    int N=scheduleResponse->getSegmentsArraySize();
    for (int i=0; i<N; ++i) {
        addSegmentToCache(scheduleResponse->getSegments(i));
    }
}

void HTopology::handleSwitchToRescueModeCall (BaseCallMessage *msg) {
    numSentMessages[ESwitchToRescueMode]++;

    HSwitchToRescueModeCall *switchCall = (HSwitchToRescueModeCall *)msg;
    if (switchCall->getSrcNode() != grandParent.getHandle()) {
        EV << "There's some problem with switchToRescueModeCall" << endl;
        return;
    }

    // TODO we can send our capacity this way, in response to this message
    modeOfOperation = RESCUE_MODE;
}

void HTopology::handleGetParametersCall (BaseCallMessage *msg) {
    numSentMessages[EGetParameters]++;

    EV << "Got a getParameters call from " << msg->getSrcNode() << endl;

    HGetParametersCall *mrpc = (HGetParametersCall *)msg;
    HGetParametersResponse *rrpc = new HGetParametersResponse ();
    rrpc->setCapacity(capacity());
    rrpc->setBandwidth(bandwidth);
    rrpc->setRescueCapacity(rescueCapacity());
    rrpc->setBitLength(HGETPARAMETERSRESPONSE_L(rrpc));

    sendRpcResponse(mrpc, rrpc);
}

void HTopology::handleGetParametersResponse (BaseResponseMessage *msg, simtime_t rtt) {
    numRecvMessages[EGetParameters]++;

    HGetParametersResponse *mrpc = (HGetParametersResponse*) msg;
    OverlayKey key = mrpc->getSrcNode().getKey();

    EV << "rtt: " << rtt.inUnit(SIMTIME_MS)
            << "capacity: " << mrpc->getCapacity()
            << ", rescue capacity: "<< mrpc->getRescueCapacity()
            << ", bandwidth: "<<  mrpc->getBandwidth()<< endl;

    if (ancestors.find(key) != ancestors.end()) {
        RankingParameters parameters = {rtt.inUnit(SIMTIME_MS), mrpc->getCapacity(), mrpc->getRescueCapacity(), mrpc->getBandwidth()};
        ancestors[key].setRankingParameters(parameters);
        parametersResponseReceived++;
        EV << "Updating parameters at entry: " << ancestors[key].getHandle()
                        << " && rank is " << ancestors[key].getRank() << endl;
    }
    else if (nodesOneUp.find(key) != nodesOneUp.end()) {
        RankingParameters parameters = {rtt.inUnit(SIMTIME_MS), mrpc->getCapacity(), mrpc->getRescueCapacity(), mrpc->getBandwidth()};
        nodesOneUp[key].setRankingParameters(parameters);
        parametersResponseReceived++;
        EV << "Updating parameters at entry: " << nodesOneUp[key].getHandle()
                << " && rank is " << nodesOneUp[key].getRank() << endl;
    }

    if (parametersResponseReceived >= parametersResponseRequired-PARAMETERS_RESPONSE_BUFFER) {
        EV << "Restarting the parameters estimation timer " << endl;
        initializedRescueRanks = true;
        // TODO call the ranking function
        // AFAIK, every time you get a new message after the buffer #of responses,
        // you'll remove the previous one & reschedule the new one => you'll ultimately have a new one
        scheduleTimer(rescueParametersTimer, rescueParameterEstimationRate);
    }
}

void HTopology::handleCapacityCall (BaseCallMessage *msg) {
    numSentMessages[ECapacity]++;

    HCapacityCall *mrpc = (HCapacityCall*)msg;          // get Call message
    HCapacityResponse *rrpc = new HCapacityResponse();  // create response
    rrpc->setRespondingNode(thisNode);

    // TODO the capacity can be decided dynamically, but for now lets use the static version
    rrpc->setCapacity(capacity());      // set the capacity left at this node
    rrpc->setBitLength(HCAPACITYRESPONSE_L(rrpc));

    // now send the response. sendRpcResponse can automatically tell where to send it to.
    // note that sendRpcResponse will delete mrpc (aka msg)!
    sendRpcResponse(mrpc, rrpc);
}

// RPC
bool HTopology::handleRpcCall(BaseCallMessage *msg) {
    // There are many macros to simplify the handling of RPCs. The full list is in <OverSim>/src/common/RpcMacros.h.
    // start a switch
    RPC_SWITCH_START(msg);

    // enters the following block if the message is of type HCapacityCall (note the shortened parameter!)
    RPC_ON_CALL(HCapacity) {
        handleCapacityCall(msg);
        RPC_HANDLED = true;  // set to true, since we did handle this RPC (default is false)
        break;
    }

    RPC_ON_CALL(HGetParameters) {
        handleGetParametersCall(msg);
        RPC_HANDLED = true;
        break;
    }

    RPC_ON_CALL(HSelectParent) {
        numSentMessages[ESelectParent]++;
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

    RPC_ON_CALL(HScheduleSegments) {
        handleScheduleSegmentsCall (msg);
        RPC_HANDLED = true;
        break;
    }

    RPC_ON_CALL(HSwitchToRescueMode) {
        handleSwitchToRescueModeCall(msg);
        RPC_HANDLED=true;
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
void HTopology::handleRpcTimeout(BaseCallMessage* msg,
                                 const TransportAddress& dest,
                                 cPolymorphic* context, int rpcId,
                                 const OverlayKey& destKey) {
    // Same macros as in handleRpc

    // start a switch
    RPC_SWITCH_START(msg);

        // DONE this timeout seems to overshooting the actual time required to send to the children
        //  - problem was that there was no response message which can negate the timeout period & hence a response was created
        // I think you should change the timeout period associated with this call, otherwise it'll assume the node is DEAD
        RPC_ON_CALL(HVideoSegment) {
            HVideoSegmentCall *mrpc = (HVideoSegmentCall *)msg;
            if (children.find(destKey) == children.end()) {
                EV << "not my child, why Am i sending it a video segment?" << endl;
            }
            else {
                EV << "Node failed is my child: " << children[destKey] << endl;

                //  Notify the failure to the children
                NodeVector nodeChildren = children[destKey].getNodeVector();
                for (size_t i=0; i<nodeChildren.size(); ++i) {
                    HSwitchToRescueModeCall *switchCall = new HSwitchToRescueModeCall();
                    switchCall->setBitLength(HSWITCHTORESCUEMODECALL_L(switchCall));
                    sendRouteRpcCall (OVERLAY_COMP, nodeChildren[i], nodeChildren[i].getKey(), switchCall);
                }

                //  Select a replacement
                selectReplacement(children[destKey].getHandle(), NULL);
            }
        }

        RPC_ON_CALL(HGetParameters) {
            EV << "TIMEOUT HGETPARAMETERS: couldn't send to " << dest << endl;
        }
    // end the switch
    RPC_SWITCH_END();
}

void HTopology::handleCapacityResponse (BaseResponseMessage *msg) {
    numRecvMessages[ECapacity]++;

    HCapacityResponse *mrpc = (HCapacityResponse*)msg;          // get Response message
    //if (queryNodesSelectionAlgo.find(mrpc->getSrcNode().getKey()) != queryNodesSelectionAlgo.end())
    // set the capacity for the key

    OverlayKey key = mrpc->getParentNode().getKey();
    if (leaveRequests.find(key) == leaveRequests.end()) {
        // TODO NOT QUITE SURE WHAT SHOULD BE DONE?
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

void HTopology::handleJoinResponse (BaseResponseMessage *msg) {
    numRecvMessages[EJoin]++;

    HJoinResponse* mrpc = (HJoinResponse*)msg;          // get Response message
    if (mrpc->getJoined() == true) {
        joinAcceptanceTime = simTime();                 // FILL IN THE JOIN ACCEPTANCE TIME

        EV << "We got a response from " << mrpc->getSrcNode() << endl;
        parent.setHandle(mrpc->getSrcNode());

        if (mrpc->getAncestorsArraySize() > 0)
            grandParent.setHandle(mrpc->getAncestors(mrpc->getAncestorsArraySize()-1));

        successorNode.setHandle(mrpc->getSuccessorNode());
        predecessorNode.setHandle(mrpc->getPredecessorNode());

        for (size_t i=0; i<mrpc->getAncestorsArraySize(); ++i) {
            NodeHandle node = mrpc->getAncestors(i);
            RescueNode hnode;
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

        RPC_ON_RESPONSE(HGetParameters) {
            handleGetParametersResponse(msg, rtt);
        }

        RPC_ON_RESPONSE (HVideoSegment) {
            numRecvMessages[EVideoSegment]++;
            EV << "got an hVideoSegment call's response" << endl;
        }

        RPC_ON_RESPONSE(HScheduleSegments) {
            handleScheduleSegmentsResponse(msg);
        }

        RPC_ON_RESPONSE(HSelectParent) {
            numRecvMessages[ESelectParent]++;
            // TODO, will there really be any response message of this sort?
        }

        RPC_ON_RESPONSE(HGetChildren) {
            setNodesOneUp(msg);
        }

        RPC_ON_RESPONSE(HJoin) {
            handleJoinResponse(msg);
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
        // TODO actually one less -> leave itself
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
        NodeVector::iterator pos=newChildren.begin();
        for (; pos!=newChildren.end(); ++pos) {
            if ((*pos).getKey() == (*it).first) break;
        }

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

        for (size_t i=0; i<newChildren.size(); ++i)
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
    // TODO RankRescueNodes : I guess this method has been implemented with some other name
    // Need to decide on some transfer characteristics in order to rank the nodes
    // same can be used for selection of parent in this kind of scenario
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

// returns the ranked nodes in their decreasing ranking order
vector<NodeHandle> HTopology::getRankedRescueNodes () {
    // TODO how do we make sure that the parameters are initialized?

    vector<RescueNode> rescueNodes;
    for(RescueMapIterator it=ancestors.begin(); it!=ancestors.end(); ++it) {
        rescueNodes.push_back((*it).second);
    }

    for(RescueMapIterator it=nodesOneUp.begin(); it!=nodesOneUp.end(); ++it) {
        rescueNodes.push_back((*it).second);
    }

    // sort them as per their ranking
    sort(rescueNodes.begin(), rescueNodes.end(), compareRescueNodes);
    vector<NodeHandle> rankedNodes;
    for (size_t i=0; i<rescueNodes.size(); ++i) rankedNodes.push_back(rescueNodes[i].getHandle());

    return rankedNodes;
}

// look for alternatives on deadline approaching segments
void HTopology::scheduleDeadlineSegments (int startSegmentID, int count, int perNode) {
    vector<NodeHandle> rankedNodes = getRankedRescueNodes();
    int nodeNo=0;

    while (count !=0) {
        // Prepare the function call
        HScheduleSegmentsCall *scheduleCall = new HScheduleSegmentsCall();
        scheduleCall->setStartSegmentID(startSegmentID);
        scheduleCall->setCount(perNode);
        scheduleCall->setBitLength(HSCHEDULESEGMENTSCALL_L(scheduleCall));

        // TODO send to the node in the order of priority
        // 1) should the bandwidth be of concern too?
        // 2) what about the timeRemaining to deadline?
        sendRouteRpcCall(OVERLAY_COMP, rankedNodes[nodeNo++], scheduleCall);

        startSegmentID += perNode;
        count -= perNode;
    }
}

// Advance Features
void HTopology::optimizeTree () {
    // TODO Deals with height consideration of the tree
}

void HTopology::calculateResourceAllocationPolicy () {
    // TODO Resource allocation policy in case we're left with limited resources to spare[see Anysee]
}
