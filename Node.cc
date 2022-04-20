#include "Node.h"

void Node::broadcastMsg(cMessage* msg)
{
    for(int i = 0; i < gateSize("out"); i++)
    {
        send(msg->dup(),"out",i);
    }
}

void Node::initialize()
{
    ID = getId() - 2;
    NodeNumber = getParentModule()->par("NodeNumber");
    maxRound = getParentModule()->par("roundNumber");

    msgPrePropose = new cMessage("Preproposing a new block",PRE_PROPOSE);
    msgPropose = new cMessage("Sending my proposal",PROPOSE);
    msgVote = new cMessage("Sending my vote",VOTE);
    msgWait = new cMessage("Waiting the current phase to finish...",WAIT);
    msgSync = new cMessage("Synchronizing for the next round...",SYNC);
    msgNext = new cMessage("Beginning a new round...",NEXT);
    
    getDisplayString().setTagArg("i",1,"red");

    if(ID == currentProposer && maxRound != 0)
    {
        getDisplayString().setTagArg("t", 0, "Proposer");
        scheduleAt(simTime(), msgNext);
        return;
    }
    
    getDisplayString().setTagArg("t", 0, "Validator");
}

void Node::casePRE_PROPOSE(cMessage* msg)
{
    //Always check the source of the msg : it's important to be send by only the current proposer
    //If it is not the case ignore the msg and return

    delete msg;

    if(false) //condition on who is sending the preproposal
    {
        return;
    }

    EV << "Checking the preproposal block...\n";

    getDisplayString().setTagArg("i",1,"orange");

    //Check the preproposal block

    broadcastMsg(msgPropose); //Broadcast the result
}

void Node::casePROPOSE(cMessage* msg)
{
    //For each validator store the first received proposal, if a validator send another proposal ignore it 
    
    delete msg;
    
    EV << "Getting the proposals of other nodes...\n";

    //Add the received proposals
}

void Node::caseVOTE(cMessage* msg)
{
    //Always check if it is the proposer that is initiating the vote 
    //The node have to received the first vote by the proposer, if it is not the case ignore the msg
    //For each validator store the first received vote, if a validator send another vote ignore it 
    
    delete msg;

    if(true) //condition on who is initiating the vote
    {
        
    }

    if(!haveVoted)
    {
        haveVoted = true;
        getDisplayString().setTagArg("i",1,"green");
        broadcastMsg(msgVote);
    }
    
    EV << "Getting the votes of other nodes...\n";

    //Stores the votes
    
    return;
}

void Node::caseSYNC(cMessage* msg)
{
    //Always check if it is the proposer that is initiating the sync 
    //If it is not the case ignore the msg
    
    delete msg;

    if(true) //condition on who is initiating the sync
    {
        
    }
    
    EV << "Syncing...\n";

    getDisplayString().setTagArg("i",1,"black");

    //Compute the result based on the current votes (with the 2f + 1 value)
    //Store it
    
    haveVoted = false;
    roundCount++;
    currentProposer = currentProposer >= NodeNumber ? 0 : currentProposer + 1;

    if(currentProposer == ID && roundCount < maxRound)
    {
        getDisplayString().setTagArg("t", 0, "Proposer");
        scheduleAt(simTime() + par("waitTime"), msgNext);
    }

    getDisplayString().setTagArg("i",1,"red");
}

void Node::caseWAIT(cMessage* msg)
{
    if(waitType == 0)
    {
        EV << "Proposal phase finished... Next phase : voting\n";

        getDisplayString().setTagArg("i",1,"green");

        //Compute the vote based on the current proposals (with the 2f + 1 value)
        
        broadcastMsg(msgVote); //Broadcast the vote 
        haveVoted = true;
        waitType = 1;
        scheduleAt(simTime() + par("waitTime"), msgWait);
        return;
    }

    if(waitType == 1)
    {
        EV << "Voting phase finished... Next phase : syncing\n";

        getDisplayString().setTagArg("i",1,"black");
        
        //Compute the result based on the current votes (with the 2f + 1 value)
        //Store it

        //Sync phase for the proposer
        haveVoted = false;
        roundCount++;
        currentProposer = currentProposer >= NodeNumber ? 0 : currentProposer + 1;

        getDisplayString().setTagArg("t", 0, "Validator");
        getDisplayString().setTagArg("i",1,"red");

        broadcastMsg(msgSync);//Broadcast the sync 
        return;
    }
}

void Node::caseNEXT(cMessage* msg)
{
    EV << "Beginning round " << roundCount << " with node " << ID << "\n";
    getDisplayString().setTagArg("i",1,"orange");
    waitType = 0;
    broadcastMsg(msgPrePropose);
    scheduleAt(simTime() + par("waitTime"), msgWait); 
}

void Node::handleMessage(cMessage* msg)
{
    switch(msg->getKind())
    {
        case MessageType::PRE_PROPOSE:
            casePRE_PROPOSE(msg);
            break;

        case MessageType::PROPOSE:
            casePROPOSE(msg);
            break;

        case MessageType::VOTE:
            caseVOTE(msg);
            break;

        case MessageType::WAIT:
            caseWAIT(msg);
            break;

        case MessageType::SYNC:
            caseSYNC(msg);
            break;

        case MessageType::NEXT:
            caseNEXT(msg);
            break;
    
        default:
            break;
    }  
}

void Node::finish()
{
    delete msgPrePropose;
    delete msgPropose;
    delete msgVote;
    delete msgWait;
    delete msgSync;
    delete msgNext;
}