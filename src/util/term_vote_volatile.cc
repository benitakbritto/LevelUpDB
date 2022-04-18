#include "term_vote_volatile.h"

void VolatileTermVote::UpdateCurrentTerm(int term)
{
    if (current_term > term)
    {
        throw ("[ERROR]: Cannot decrement current term");
    }
    
    current_term = term;
}

int VolatileTermVote::GetCurrentTerm()
{
    return current_term;
}

void VolatileTermVote::AddVotedFor(int term, string ip)
{
    if (votedFor.count(term) != 0)
    {
        throw runtime_error("[ERROR]: Cannot revote in the same term");
    }

    votedFor[term] = ip;
}

string VolatileTermVote::GetVotedFor(int term)
{
    if (votedFor.count(term) == 0)
    {
        return "";
    }

    return votedFor[term];
}

bool VolatileTermVote::HasNotVoted(int term)
{
    return (votedFor.count(term) == 0) ? false : true;
}

// Tester
// int main()
// {
//     VolatileTermVote obj; 

//     obj.UpdateCurrentTerm(1);
//     dbgprintf("Current Term = %d\n", obj.GetCurrentTerm());
    
//     dbgprintf("Has voted = %d\n", obj.HasVoted(1));

//     obj.AddVotedFor(1, "Node1");
//     dbgprintf("Has voted = %d\n", obj.HasVoted(1));
//     dbgprintf("Voted for = %s\n", obj.GetVotedFor(1).c_str());

//     obj.AddVotedFor(1, "Node2"); // should fail
//     return 0;
// }