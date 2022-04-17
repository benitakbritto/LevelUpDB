struct Follower {};
struct Candidate {};
struct Leader {};

enum CurrentState { LEADER = 0, FOLLOWER = 1, CANDIDATE = 2};
