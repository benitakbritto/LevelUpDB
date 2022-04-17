struct Follower {};
struct Candidate {};
struct Leader {};

enum CurrentState { FOLLOWER = 0, CANDIDATE = 1, LEADER = 2};
