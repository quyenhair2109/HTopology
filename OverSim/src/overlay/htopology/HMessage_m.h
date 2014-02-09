//
// Generated file, do not edit! Created by opp_msgc 4.3 from overlay/htopology/HMessage.msg.
//

#ifndef _HMESSAGE_M_H_
#define _HMESSAGE_M_H_

#include <omnetpp.h>

// opp_msgc version check
#define MSGC_VERSION 0x0403
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgc: 'make clean' should help.
#endif

// cplusplus {{
#include <TransportAddress.h>
#include <NodeHandle.h>
#include <OverlayKey.h>
#include <CommonMessages_m.h>
#include "HNode.h"
#include "HStructs.h"

#define JOINCALL_L(msg) 					BASECALL_L(msg)
#define HJOINRESPONSE_L(msg) 				(BASERESPONSE_L(msg) + 2* NODEHANDLE_L +\
                             					(msg->getAncestorsArraySize() * NODEHANDLE_L) + TYPE_L)
        
#define HGETCHILDRENCALL_L(msg)				BASECALL_L(msg)
#define HGETCHILDRENRESPONSE_L(msg) 		(BASERESPONSE_L(msg) + (msg->getChildrenArraySize() * NODEHANDLE_L))
#define HCAPACITYCALL_L(msg) 				BASECALL_L(msg) + KEY_L
#define HCAPACITYRESPONSE_L(msg) 			(BASERESPONSE_L(msg) + (2*NODEHANDLE_L) + TYPE_L)
#define HSELECTPARENTCALL_L(msg) 			HCAPACITYCALL_L(msg)
#define HSELECTPARENTRESPONSE_L(msg) 		BASERESPONSE_L(msg) + NODEHANDLE_L



#define HVIDEOSEGMENTCALL_L(msg)			(BASECALL_L(msg) + TYPE_L + SEGMENT_SIZE * TYPE_L)

#define HLEAVEOVERLAYCALL_L(msg)			BASECALL_L(msg)
#define HLEAVEOVERLAYRESPONSE_L(msg) 		BASERESPONSE_L(msg) + TYPE_L

#define HNEWPARENTSELECTEDCALL_L(msg)		(BASECALL_L(msg) + NODEHANDLE_L)
#define HRESPONSIBILITYASPARENTCALL_L(msg)  (BASECALL_L(msg) + (msg->getChildrenArraySize()+1) * NODEHANDLE_L)
		


#define HVIDEOSEGMENT_L						((SEGMENT_SIZE+1) * TYPE_L)
#define HSCHEDULESEGMENTSCALL_L(msg)		(BASECALL_L(msg) + 2*TYPE_L)
#define HSCHEDULESEGMENTSRESPONSE_L(msg)	(BASERESPONSE_L(msg) + (msg->getSegmentsArraySize()) * HVIDEOSEGMENT_L)
// }}



/**
 * Enum generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * enum MessageType {
 *     M_JOIN=1;				
 *     M_LEAVE=2;			
 *     M_QUERY_NODE=3;		
 *     M_QUERY_SEGMENT=4;	
 *     M_RESCUE=5;			
 * };
 * </pre>
 */
enum MessageType {
    M_JOIN = 1,
    M_LEAVE = 2,
    M_QUERY_NODE = 3,
    M_QUERY_SEGMENT = 4,
    M_RESCUE = 5
};

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HCapacityCall extends BaseCallMessage {
 *     OverlayKey destinationKey;
 * };
 * </pre>
 */
class HCapacityCall : public ::BaseCallMessage
{
  protected:
    OverlayKey destinationKey_var;

  private:
    void copy(const HCapacityCall& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HCapacityCall&);

  public:
    HCapacityCall(const char *name=NULL, int kind=0);
    HCapacityCall(const HCapacityCall& other);
    virtual ~HCapacityCall();
    HCapacityCall& operator=(const HCapacityCall& other);
    virtual HCapacityCall *dup() const {return new HCapacityCall(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual OverlayKey& getDestinationKey();
    virtual const OverlayKey& getDestinationKey() const {return const_cast<HCapacityCall*>(this)->getDestinationKey();}
    virtual void setDestinationKey(const OverlayKey& destinationKey);
};

inline void doPacking(cCommBuffer *b, HCapacityCall& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HCapacityCall& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HCapacityResponse extends BaseResponseMessage {
 *     NodeHandle parentNode;
 *     NodeHandle respondingNode;
 *     int capacity;						
 * };
 * </pre>
 */
class HCapacityResponse : public ::BaseResponseMessage
{
  protected:
    NodeHandle parentNode_var;
    NodeHandle respondingNode_var;
    int capacity_var;

  private:
    void copy(const HCapacityResponse& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HCapacityResponse&);

  public:
    HCapacityResponse(const char *name=NULL, int kind=0);
    HCapacityResponse(const HCapacityResponse& other);
    virtual ~HCapacityResponse();
    HCapacityResponse& operator=(const HCapacityResponse& other);
    virtual HCapacityResponse *dup() const {return new HCapacityResponse(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual NodeHandle& getParentNode();
    virtual const NodeHandle& getParentNode() const {return const_cast<HCapacityResponse*>(this)->getParentNode();}
    virtual void setParentNode(const NodeHandle& parentNode);
    virtual NodeHandle& getRespondingNode();
    virtual const NodeHandle& getRespondingNode() const {return const_cast<HCapacityResponse*>(this)->getRespondingNode();}
    virtual void setRespondingNode(const NodeHandle& respondingNode);
    virtual int getCapacity() const;
    virtual void setCapacity(int capacity);
};

inline void doPacking(cCommBuffer *b, HCapacityResponse& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HCapacityResponse& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HSelectParentCall extends BaseCallMessage {
 *     OverlayKey key;
 * };
 * </pre>
 */
class HSelectParentCall : public ::BaseCallMessage
{
  protected:
    OverlayKey key_var;

  private:
    void copy(const HSelectParentCall& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HSelectParentCall&);

  public:
    HSelectParentCall(const char *name=NULL, int kind=0);
    HSelectParentCall(const HSelectParentCall& other);
    virtual ~HSelectParentCall();
    HSelectParentCall& operator=(const HSelectParentCall& other);
    virtual HSelectParentCall *dup() const {return new HSelectParentCall(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual OverlayKey& getKey();
    virtual const OverlayKey& getKey() const {return const_cast<HSelectParentCall*>(this)->getKey();}
    virtual void setKey(const OverlayKey& key);
};

inline void doPacking(cCommBuffer *b, HSelectParentCall& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HSelectParentCall& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HSelectParentResponse extends BaseResponseMessage {
 *     NodeHandle respondingNode;
 *     
 * };
 * </pre>
 */
class HSelectParentResponse : public ::BaseResponseMessage
{
  protected:
    NodeHandle respondingNode_var;

  private:
    void copy(const HSelectParentResponse& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HSelectParentResponse&);

  public:
    HSelectParentResponse(const char *name=NULL, int kind=0);
    HSelectParentResponse(const HSelectParentResponse& other);
    virtual ~HSelectParentResponse();
    HSelectParentResponse& operator=(const HSelectParentResponse& other);
    virtual HSelectParentResponse *dup() const {return new HSelectParentResponse(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual NodeHandle& getRespondingNode();
    virtual const NodeHandle& getRespondingNode() const {return const_cast<HSelectParentResponse*>(this)->getRespondingNode();}
    virtual void setRespondingNode(const NodeHandle& respondingNode);
};

inline void doPacking(cCommBuffer *b, HSelectParentResponse& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HSelectParentResponse& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HJoinCall extends BaseCallMessage {
 * }
 * </pre>
 */
class HJoinCall : public ::BaseCallMessage
{
  protected:

  private:
    void copy(const HJoinCall& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HJoinCall&);

  public:
    HJoinCall(const char *name=NULL, int kind=0);
    HJoinCall(const HJoinCall& other);
    virtual ~HJoinCall();
    HJoinCall& operator=(const HJoinCall& other);
    virtual HJoinCall *dup() const {return new HJoinCall(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
};

inline void doPacking(cCommBuffer *b, HJoinCall& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HJoinCall& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HJoinResponse extends BaseResponseMessage {
 *     
 *     
 *     
 *     
 *     
 *     NodeHandle ancestors[];  	
 *     NodeHandle successorNode;
 *     NodeHandle predecessorNode;
 *     int joined;
 * }
 * </pre>
 */
class HJoinResponse : public ::BaseResponseMessage
{
  protected:
    NodeHandle *ancestors_var; // array ptr
    unsigned int ancestors_arraysize;
    NodeHandle successorNode_var;
    NodeHandle predecessorNode_var;
    int joined_var;

  private:
    void copy(const HJoinResponse& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HJoinResponse&);

  public:
    HJoinResponse(const char *name=NULL, int kind=0);
    HJoinResponse(const HJoinResponse& other);
    virtual ~HJoinResponse();
    HJoinResponse& operator=(const HJoinResponse& other);
    virtual HJoinResponse *dup() const {return new HJoinResponse(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual void setAncestorsArraySize(unsigned int size);
    virtual unsigned int getAncestorsArraySize() const;
    virtual NodeHandle& getAncestors(unsigned int k);
    virtual const NodeHandle& getAncestors(unsigned int k) const {return const_cast<HJoinResponse*>(this)->getAncestors(k);}
    virtual void setAncestors(unsigned int k, const NodeHandle& ancestors);
    virtual NodeHandle& getSuccessorNode();
    virtual const NodeHandle& getSuccessorNode() const {return const_cast<HJoinResponse*>(this)->getSuccessorNode();}
    virtual void setSuccessorNode(const NodeHandle& successorNode);
    virtual NodeHandle& getPredecessorNode();
    virtual const NodeHandle& getPredecessorNode() const {return const_cast<HJoinResponse*>(this)->getPredecessorNode();}
    virtual void setPredecessorNode(const NodeHandle& predecessorNode);
    virtual int getJoined() const;
    virtual void setJoined(int joined);
};

inline void doPacking(cCommBuffer *b, HJoinResponse& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HJoinResponse& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HVideoSegmentCall extends BaseCallMessage {
 *     HVideoSegment segment;
 * }
 * </pre>
 */
class HVideoSegmentCall : public ::BaseCallMessage
{
  protected:
    HVideoSegment segment_var;

  private:
    void copy(const HVideoSegmentCall& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HVideoSegmentCall&);

  public:
    HVideoSegmentCall(const char *name=NULL, int kind=0);
    HVideoSegmentCall(const HVideoSegmentCall& other);
    virtual ~HVideoSegmentCall();
    HVideoSegmentCall& operator=(const HVideoSegmentCall& other);
    virtual HVideoSegmentCall *dup() const {return new HVideoSegmentCall(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual HVideoSegment& getSegment();
    virtual const HVideoSegment& getSegment() const {return const_cast<HVideoSegmentCall*>(this)->getSegment();}
    virtual void setSegment(const HVideoSegment& segment);
};

inline void doPacking(cCommBuffer *b, HVideoSegmentCall& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HVideoSegmentCall& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HLeaveOverlayCall extends BaseCallMessage {
 * }
 * </pre>
 */
class HLeaveOverlayCall : public ::BaseCallMessage
{
  protected:

  private:
    void copy(const HLeaveOverlayCall& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HLeaveOverlayCall&);

  public:
    HLeaveOverlayCall(const char *name=NULL, int kind=0);
    HLeaveOverlayCall(const HLeaveOverlayCall& other);
    virtual ~HLeaveOverlayCall();
    HLeaveOverlayCall& operator=(const HLeaveOverlayCall& other);
    virtual HLeaveOverlayCall *dup() const {return new HLeaveOverlayCall(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
};

inline void doPacking(cCommBuffer *b, HLeaveOverlayCall& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HLeaveOverlayCall& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HLeaveOverlayResponse extends BaseResponseMessage {
 *     int permissionGranted;
 * }
 * </pre>
 */
class HLeaveOverlayResponse : public ::BaseResponseMessage
{
  protected:
    int permissionGranted_var;

  private:
    void copy(const HLeaveOverlayResponse& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HLeaveOverlayResponse&);

  public:
    HLeaveOverlayResponse(const char *name=NULL, int kind=0);
    HLeaveOverlayResponse(const HLeaveOverlayResponse& other);
    virtual ~HLeaveOverlayResponse();
    HLeaveOverlayResponse& operator=(const HLeaveOverlayResponse& other);
    virtual HLeaveOverlayResponse *dup() const {return new HLeaveOverlayResponse(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual int getPermissionGranted() const;
    virtual void setPermissionGranted(int permissionGranted);
};

inline void doPacking(cCommBuffer *b, HLeaveOverlayResponse& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HLeaveOverlayResponse& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HNewParentSelectedCall extends BaseCallMessage {
 *     NodeHandle parent;
 * }
 * </pre>
 */
class HNewParentSelectedCall : public ::BaseCallMessage
{
  protected:
    NodeHandle parent_var;

  private:
    void copy(const HNewParentSelectedCall& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HNewParentSelectedCall&);

  public:
    HNewParentSelectedCall(const char *name=NULL, int kind=0);
    HNewParentSelectedCall(const HNewParentSelectedCall& other);
    virtual ~HNewParentSelectedCall();
    HNewParentSelectedCall& operator=(const HNewParentSelectedCall& other);
    virtual HNewParentSelectedCall *dup() const {return new HNewParentSelectedCall(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual NodeHandle& getParent();
    virtual const NodeHandle& getParent() const {return const_cast<HNewParentSelectedCall*>(this)->getParent();}
    virtual void setParent(const NodeHandle& parent);
};

inline void doPacking(cCommBuffer *b, HNewParentSelectedCall& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HNewParentSelectedCall& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HResponsibilityAsParentCall extends BaseCallMessage {
 *     NodeHandle parent;
 *     NodeHandle children[];
 * }
 * </pre>
 */
class HResponsibilityAsParentCall : public ::BaseCallMessage
{
  protected:
    NodeHandle parent_var;
    NodeHandle *children_var; // array ptr
    unsigned int children_arraysize;

  private:
    void copy(const HResponsibilityAsParentCall& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HResponsibilityAsParentCall&);

  public:
    HResponsibilityAsParentCall(const char *name=NULL, int kind=0);
    HResponsibilityAsParentCall(const HResponsibilityAsParentCall& other);
    virtual ~HResponsibilityAsParentCall();
    HResponsibilityAsParentCall& operator=(const HResponsibilityAsParentCall& other);
    virtual HResponsibilityAsParentCall *dup() const {return new HResponsibilityAsParentCall(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual NodeHandle& getParent();
    virtual const NodeHandle& getParent() const {return const_cast<HResponsibilityAsParentCall*>(this)->getParent();}
    virtual void setParent(const NodeHandle& parent);
    virtual void setChildrenArraySize(unsigned int size);
    virtual unsigned int getChildrenArraySize() const;
    virtual NodeHandle& getChildren(unsigned int k);
    virtual const NodeHandle& getChildren(unsigned int k) const {return const_cast<HResponsibilityAsParentCall*>(this)->getChildren(k);}
    virtual void setChildren(unsigned int k, const NodeHandle& children);
};

inline void doPacking(cCommBuffer *b, HResponsibilityAsParentCall& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HResponsibilityAsParentCall& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HScheduleSegmentsCall extends BaseCallMessage {
 *    int startSegmentID;
 *    int count; 
 * }
 * </pre>
 */
class HScheduleSegmentsCall : public ::BaseCallMessage
{
  protected:
    int startSegmentID_var;
    int count_var;

  private:
    void copy(const HScheduleSegmentsCall& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HScheduleSegmentsCall&);

  public:
    HScheduleSegmentsCall(const char *name=NULL, int kind=0);
    HScheduleSegmentsCall(const HScheduleSegmentsCall& other);
    virtual ~HScheduleSegmentsCall();
    HScheduleSegmentsCall& operator=(const HScheduleSegmentsCall& other);
    virtual HScheduleSegmentsCall *dup() const {return new HScheduleSegmentsCall(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual int getStartSegmentID() const;
    virtual void setStartSegmentID(int startSegmentID);
    virtual int getCount() const;
    virtual void setCount(int count);
};

inline void doPacking(cCommBuffer *b, HScheduleSegmentsCall& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HScheduleSegmentsCall& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HScheduleSegmentsResponse extends BaseResponseMessage {
 *     HVideoSegment segments[];
 * }
 * </pre>
 */
class HScheduleSegmentsResponse : public ::BaseResponseMessage
{
  protected:
    HVideoSegment *segments_var; // array ptr
    unsigned int segments_arraysize;

  private:
    void copy(const HScheduleSegmentsResponse& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HScheduleSegmentsResponse&);

  public:
    HScheduleSegmentsResponse(const char *name=NULL, int kind=0);
    HScheduleSegmentsResponse(const HScheduleSegmentsResponse& other);
    virtual ~HScheduleSegmentsResponse();
    HScheduleSegmentsResponse& operator=(const HScheduleSegmentsResponse& other);
    virtual HScheduleSegmentsResponse *dup() const {return new HScheduleSegmentsResponse(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual void setSegmentsArraySize(unsigned int size);
    virtual unsigned int getSegmentsArraySize() const;
    virtual HVideoSegment& getSegments(unsigned int k);
    virtual const HVideoSegment& getSegments(unsigned int k) const {return const_cast<HScheduleSegmentsResponse*>(this)->getSegments(k);}
    virtual void setSegments(unsigned int k, const HVideoSegment& segments);
};

inline void doPacking(cCommBuffer *b, HScheduleSegmentsResponse& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HScheduleSegmentsResponse& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HGetChildrenCall extends BaseCallMessage {
 * }
 * </pre>
 */
class HGetChildrenCall : public ::BaseCallMessage
{
  protected:

  private:
    void copy(const HGetChildrenCall& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HGetChildrenCall&);

  public:
    HGetChildrenCall(const char *name=NULL, int kind=0);
    HGetChildrenCall(const HGetChildrenCall& other);
    virtual ~HGetChildrenCall();
    HGetChildrenCall& operator=(const HGetChildrenCall& other);
    virtual HGetChildrenCall *dup() const {return new HGetChildrenCall(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
};

inline void doPacking(cCommBuffer *b, HGetChildrenCall& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HGetChildrenCall& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HGetChildrenResponse extends BaseResponseMessage {
 *     NodeHandle children[];
 * }
 * </pre>
 */
class HGetChildrenResponse : public ::BaseResponseMessage
{
  protected:
    NodeHandle *children_var; // array ptr
    unsigned int children_arraysize;

  private:
    void copy(const HGetChildrenResponse& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HGetChildrenResponse&);

  public:
    HGetChildrenResponse(const char *name=NULL, int kind=0);
    HGetChildrenResponse(const HGetChildrenResponse& other);
    virtual ~HGetChildrenResponse();
    HGetChildrenResponse& operator=(const HGetChildrenResponse& other);
    virtual HGetChildrenResponse *dup() const {return new HGetChildrenResponse(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual void setChildrenArraySize(unsigned int size);
    virtual unsigned int getChildrenArraySize() const;
    virtual NodeHandle& getChildren(unsigned int k);
    virtual const NodeHandle& getChildren(unsigned int k) const {return const_cast<HGetChildrenResponse*>(this)->getChildren(k);}
    virtual void setChildren(unsigned int k, const NodeHandle& children);
};

inline void doPacking(cCommBuffer *b, HGetChildrenResponse& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HGetChildrenResponse& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>overlay/htopology/HMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet HMessage {
 *     int type enum(MessageType);			
 *     TransportAddress senderAddress;		
 *     int nodeID;							
 *     int parentID;						
 *     int optionalParameter;				
 *     string anyMessage;					
 * };
 * </pre>
 */
class HMessage : public ::cPacket
{
  protected:
    int type_var;
    TransportAddress senderAddress_var;
    int nodeID_var;
    int parentID_var;
    int optionalParameter_var;
    opp_string anyMessage_var;

  private:
    void copy(const HMessage& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HMessage&);

  public:
    HMessage(const char *name=NULL, int kind=0);
    HMessage(const HMessage& other);
    virtual ~HMessage();
    HMessage& operator=(const HMessage& other);
    virtual HMessage *dup() const {return new HMessage(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual int getType() const;
    virtual void setType(int type);
    virtual TransportAddress& getSenderAddress();
    virtual const TransportAddress& getSenderAddress() const {return const_cast<HMessage*>(this)->getSenderAddress();}
    virtual void setSenderAddress(const TransportAddress& senderAddress);
    virtual int getNodeID() const;
    virtual void setNodeID(int nodeID);
    virtual int getParentID() const;
    virtual void setParentID(int parentID);
    virtual int getOptionalParameter() const;
    virtual void setOptionalParameter(int optionalParameter);
    virtual const char * getAnyMessage() const;
    virtual void setAnyMessage(const char * anyMessage);
};

inline void doPacking(cCommBuffer *b, HMessage& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, HMessage& obj) {obj.parsimUnpack(b);}


#endif // _HMESSAGE_M_H_
