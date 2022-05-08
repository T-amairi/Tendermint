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
    maxEpoch = getParentModule()->par("epochNumber");
    f = getParentModule()->par("f");

    msgPrePropose = new cMessage("Preproposing a new block",PRE_PROPOSE);
    msgPropose = new cMessage("Sending my proposal",PROPOSE);
    msgVote = new cMessage("Sending my vote",VOTE);
    msgWait = new cMessage("Waiting the current phase to finish...",WAIT);
    msgSync = new cMessage("Synchronizing for the next round...",SYNC);
    msgNext = new cMessage("Beginning a new round...",NEXT);

    decision = new std::string("null");
    lockedValue = new std::string("null");
    validValue = new std::string("null");
    proposal = new std::string();
    v = new std::string("null");
    vote = new std::string("null");
    
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
    getDisplayString().setTagArg("i",1,"orange");

    //Get preproposal
    auto preProp = (std::string*) msg->getContextPointer();

    //Delivery phase
    *v = *preProp;
    EV << "Received the preproposal, setting v to " << *v << "\n";

    //Compute phase
    computePropose();

    //Broadcast the result (Send phase of Round Propose)
    if(*proposal != "null")
    {
        EV << "Broadcasting my proposal result...\n";
        msgPropose->setContextPointer(proposal);
        broadcastMsg(msgPropose);
    }
    
    delete msg;
}

void Node::casePROPOSE(cMessage* msg)
{
    EV << "Collecting the proposals from other nodes...\n";

    //Collect proposal during round Propose
    auto prop = (std::string*) msg->getContextPointer();

    if(propCount.find(*prop) != propCount.end())
    {
        EV << "Incrementing the proposal of " << *prop << "\n";
        propCount[*prop]++;
    }

    else
    {
        EV << "Adding " << *prop << " to the map\n";
        propCount[*prop] = 1;
    }
    
    delete msg;
}

void Node::caseVOTE(cMessage* msg)
{
    //get vote
    auto recvVote = (std::string*) msg->getContextPointer();

    //vote
    if(!haveVoted)
    {
        haveVoted = true;
        getDisplayString().setTagArg("i",1,"green");
        computeVote();

        if(*vote != "null")
        {
            EV << "Sending my vote !\n";
            msgVote->setContextPointer(vote);
            broadcastMsg(msgVote);
        }        
    }
    
    EV << "Collecting the votes from other nodes...\n";

    //Delivery phase
    if(voteCount.find(*recvVote) != voteCount.end())
    {
        EV << "Incrementing the vote of " << *recvVote << "\n";
        voteCount[*recvVote]++;
    }

    else
    {
        EV << "Adding " << *recvVote << " to the map\n";
        voteCount[*recvVote] = 1;
    }
    
    delete msg;
}

void Node::caseSYNC(cMessage* msg)
{
    EV << "Syncing and computing decision...\n";
    getDisplayString().setTagArg("i",1,"black");

    using pair_type = decltype(voteCount)::value_type;
    
    auto maxVote = std::max_element
    (
        std::begin(voteCount), std::end(voteCount),[](const pair_type & p1, const pair_type & p2){return p1.second < p2.second;}
    );

    if(!voteCount.empty() && maxVote->second >= 2*f + 1 && isValidTx(maxVote->first) && *decision == "null")
    {
        EV << "Selected " << maxVote->first << " with " << maxVote->second << " votes\n";
        *decision = maxVote->first;
    }

    else if(epochCounter < maxEpoch)
    {
        EV << "Can't make a consensus, starting a new epoch\n";
        epochCounter++;
        *v = "null";
        haveVoted = false;
        propCount.clear();
        voteCount.clear();

        if(*validValue != "null")
        {
            *proposal = *validValue;
        }

        getDisplayString().setTagArg("i",1,"red");
        delete msg;
        return;
    }

    EV << "Changing the proposer...\n";
    
    haveVoted = false;
    roundCount++;
    epochCounter = 0;
    *decision = "null";
    *lockedValue = "null";
    *validValue = "null";
    *v = "null";
    *vote = "null";
    propCount.clear();
    voteCount.clear();
    currentProposer = currentProposer >= NodeNumber ? 0 : currentProposer + 1;

    if(currentProposer == ID && roundCount < maxRound)
    {
        getDisplayString().setTagArg("t", 0, "Proposer");
        scheduleAt(simTime() + par("waitTime"), msgNext);
    }

    getDisplayString().setTagArg("i",1,"red");

    delete msg;
}

void Node::caseWAIT(cMessage* msg)
{
    if(waitType == 0)
    {
        EV << "Proposal phase finished... Next phase : voting\n";

        getDisplayString().setTagArg("i",1,"green");

        computeVote();
        msgVote->setContextPointer(vote);

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
        
        using pair_type = decltype(voteCount)::value_type;
    
        auto maxVote = std::max_element
        (
            std::begin(voteCount), std::end(voteCount),[](const pair_type & p1, const pair_type & p2){return p1.second < p2.second;}
        );

        if(!voteCount.empty() && maxVote->second >= 2*f + 1 && isValidTx(maxVote->first) && *decision == "null")
        {
            EV << "Selected " << maxVote->first << " with " << maxVote->second << " votes\n";
            *decision = maxVote->first;
        }

        else if(epochCounter < maxEpoch)
        {
            EV << "Can't make a consensus, starting a new epoch\n";
            epochCounter++;
            *v = "null";
            haveVoted = false;
            propCount.clear();
            voteCount.clear();

            if(*validValue != "null")
            {
                *proposal = *validValue;
            }

            else
            {
                getValue();
                EV << "Setting my proposal : " << *proposal << "\n"; 
                msgPrePropose->setContextPointer(proposal);
            }

            getDisplayString().setTagArg("i",1,"red");
            broadcastMsg(msgSync);//Broadcast the sync
            scheduleAt(simTime() + par("waitTime"), msgNext); 
            return;
        }

        EV << "Changing the proposer...\n";
             
        //Sync phase for the proposer
        haveVoted = false;
        roundCount++;
        epochCounter = 0;
        *decision = "null";
        *lockedValue = "null";
        *validValue = "null";
        *v = "null";
        *vote = "null";
        propCount.clear();
        voteCount.clear();
        currentProposer = currentProposer >= NodeNumber ? 0 : currentProposer + 1;

        getDisplayString().setTagArg("t", 0, "Validator");
        getDisplayString().setTagArg("i",1,"red");

        broadcastMsg(msgSync);//Broadcast the sync 
        return;
    }
}

void Node::caseNEXT(cMessage* msg)
{
    getDisplayString().setTagArg("i",1,"orange");
    waitType = 0;

    //Send phase
    if(epochCounter == 0)
    {
        EV << "Beginning round (epoch 0) " << roundCount << " with node " << ID << "\n";
        getValue();
        EV << "Setting my proposal : " << *proposal << "\n"; 
        msgPrePropose->setContextPointer(proposal);
    }

    else
    {
        EV << "Continuing round (epoch " << epochCounter << ") "<< roundCount << " with node " << ID << "\n";
    }

    EV << "Sending my proposal...\n";

    broadcastMsg(msgPrePropose);
    scheduleAt(simTime() + par("waitTime"), msgWait); 
}

void Node::getValue()
{
    //here get the ID of the Tx to propose instead of "Tx"
    *proposal = "Tx" + std::to_string(ID);
    propCount[*proposal] = 1;
}

bool Node::isValidTx(std::string Tx)
{
    if(Tx == "null") return false;

    //check Tx wrt to the ledger here
    return true;
}

void Node::computePropose()
{
    EV << "Checking the preproposal block...\n";

    if(!isValidTx(*v))
    {
        *proposal = "null";
    }

    else
    {
        if(*validValue == "null" || *lockedValue == *v || *validValue == *v)
        {
            *proposal = *v;
            propCount[*proposal] = 2;
        }

        else
        {
            *proposal = "null";
        }
    }

    EV << "Proposal equals : " << *proposal << "\n"; 
}

void Node::computeVote()
{
    using pair_type = decltype(propCount)::value_type;
    
    auto maxProp = std::max_element
    (
        std::begin(propCount), std::end(propCount),[](const pair_type & p1, const pair_type & p2){return p1.second < p2.second;}
    );

    EV << "Computing my vote...\n";

    if(maxProp->second >= 2*f + 1 && isValidTx(maxProp->first))
    {
        EV << "Selected " << maxProp->first << " with " << maxProp->second << " proposals\n";
        *lockedValue = maxProp->first;
        *validValue = maxProp->first;
        *vote = maxProp->first;
        voteCount[*vote] = 1;
    }

    else
    {
        EV << "My vote has been set to null\n";
        *vote = "null";
    }
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

    delete decision;
    delete lockedValue;
    delete validValue;
    delete proposal;
    delete v;
    delete vote;
}