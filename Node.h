#pragma once
#include <omnetpp.h>
#include <algorithm>

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

        void getValue(); //get a tx to propose
        bool isValidTx(std::string Tx); //check if Tx is valid w.r.t the ledger
        void computePropose(); //set proposal value 
        void computeVote(); //set vote value 

    private:
        int ID;
        int NodeNumber;
        int currentProposer = 0;

        int roundCount = 0;
        int maxRound;

        int epochCounter = 0;
        int maxEpoch;

        bool haveVoted = false;
        int waitType = 0;
        int f;

        std::map<std::string,int> propCount;
        std::map<std::string,int> voteCount;

        std::string* decision;
        std::string* lockedValue;
        std::string* validValue;
        std::string* proposal;
        std::string* v;
        std::string* vote;
        
        cMessage* msgPrePropose;
        cMessage* msgPropose;
        cMessage* msgVote;
        cMessage* msgWait;
        cMessage* msgSync;
        cMessage* msgNext;
};

Define_Module(Node);
