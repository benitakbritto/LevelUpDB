#include "term_vote_volatile.h"

/*
*   @brief Change current term in mem
*
*   @param term 
*/
void VolatileTermVote::UpdateCurrentTerm(int term)
{
    if (current_term > term)
    {
        throw ("[ERROR]: Cannot decrement current term");
    }
    
    current_term = term;
}

/*
*   @brief Get current term from mem
*
*   @return term
*/
int VolatileTermVote::GetCurrentTerm()
{
    return current_term;
}

/*
*   @brief Set voted for node ip to term in mem
*
*   @param  term
*   @param  ip
*/
void VolatileTermVote::AddVotedFor(int term, string ip)
{
    if (votedFor.count(term) != 0)
    {
        throw runtime_error("[ERROR]: Cannot revote in the same term");
    }

    votedFor[term] = ip;
}

/*
*   @brief Get the voted for node ip for term from mem
*
*   @param term
*   @return ip
*/
string VolatileTermVote::GetVotedFor(int term)
{
    if (votedFor.count(term) == 0)
    {
        return "";
    }

    return votedFor[term];
}

/*
*   @brief Check if already voted for a node in term
*
*   @param  term
*   @return  voted status
*/
bool VolatileTermVote::HasNotVoted(int term)
{
    return (votedFor.count(term) == 0) ? false : true;
}

/*
*   @brief Uncomment to test individually
*   @usage g++ -o tv term_vote_volatile.cc -Wall  
*/
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