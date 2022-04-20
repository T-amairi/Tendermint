#pragma once
#include <omnetpp.h>

using namespace omnetpp;

enum MessageType{PRE_PROPOSE,PROPOSE,VOTE,WAIT,SYNC,NEXT};

class Node : public cSimpleModule
{
    public:
        void broadcastMsg(cMessage* msg);

        void initialize() override;
        void handleMessage(cMessage * msg) override;
        void finish() override;

        void casePRE_PROPOSE(cMessage* msg);
        void casePROPOSE(cMessage* msg);
        void caseVOTE(cMessage* msg);
        void caseWAIT(cMessage* msg);
        void caseSYNC(cMessage* msg);
        void caseNEXT(cMessage* msg);

    private:
        int ID;
        int NodeNumber;
        int currentProposer = 0;
        int roundCount = 0;
        int maxRound;
        bool haveVoted = false;
        int waitType = 0;

        cMessage* msgPrePropose;
        cMessage* msgPropose;
        cMessage* msgVote;
        cMessage* msgWait;
        cMessage* msgSync;
        cMessage* msgNext;
};

Define_Module(Node);
