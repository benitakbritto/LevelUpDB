#include "state_helper.h"

int StateHelper::GetCurrentTerm()
{
    return vTermVoteObj.GetCurrentTerm();
}

void StateHelper::AddVotedFor(int term, string ip)
{
    if (!vTermVoteObj.HasVoted(term))
    {
        pTermVoteObj.AddVotedFor(term, ip);
        vTermVoteObj.AddVotedFor(term, ip);
    }
}

string StateHelper::GetVotedFor(int term)
{
    return vTermVoteObj.GetVotedFor(term);
}


void StateHelper::AddCurrentTerm(int term)
{
    vTermVoteObj.UpdateCurrentTerm(term);
    pTermVoteObj.AddTerm(term);
}

// TODO
void Init()
{

}

// Tester
// g++ state_helper.cc term_vote_volatile.cc term_vote_persistent.cc -Wall -o state
int main()
{
    StateHelper obj;

    cout << "Current term = " << obj.GetCurrentTerm() << endl;
    
    obj.AddCurrentTerm(1);
    cout << "Current term = " << obj.GetCurrentTerm() << endl;

    cout << "voted for in term 1 = " << obj.GetVotedFor(1) << endl;

    obj.AddVotedFor(1, "node");
    cout << "voted for in term 1 = " << obj.GetVotedFor(1) << endl;

    return 0;
}