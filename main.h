#include "engine.h"
#include <thread>





//--------------------------------
extern double time_remains;
extern double time_base;
extern unsigned moves_per_session;
extern double time_inc;
extern unsigned max_nodes_to_search;
extern unsigned max_search_depth;
extern bool stop;
extern bool _abort_;
extern bool busy;
extern u64 total_nodes;
extern double total_time_spent;
extern bool time_command_sent;
extern hash_table_c hash_table;


//extern std::vector <float> param;






//--------------------------------
struct cmdStruct
{
    std::string command;
    void (*foo)(std::string);
};





//--------------------------------
int  main(int argc, char* argv[]);
bool CmdProcess(std::string in);
bool LooksLikeMove(std::string in);
void NewCommand(std::string in);
void SetboardCommand(std::string in);
void QuitCommand(std::string in);
void PerftCommand(std::string in);
void GoCommand(std::string in);
void LevelCommand(std::string in);
void ForceCommand(std::string in);
void SetNodesCommand(std::string in);
void SetTimeCommand(std::string in);
void SetDepthCommand(std::string in);
void Unsupported(std::string in);
void GetFirstArg(std::string in, std::string *first, std::string *remain);
void ProtoverCommand(std::string in);
void StopEngine();
void StopCommand(std::string in);
void ResultCommand(std::string in);
void XboardCommand(std::string in);
void TimeCommand(std::string in);
void EvalCommand(std::string in);
void TestCommand(std::string in);
void FenCommand(std::string in);
void UciCommand(std::string in);
void SetOptionCommand(std::string in);
void IsReadyCommand(std::string in);
void PositionCommand(std::string in);
void ProcessMoveSequence(std::string in);
void UciGoCommand(std::string in);
void EasyCommand(std::string in);
void HardCommand(std::string in);
void PonderhitCommand(std::string in);
void MemoryCommand(std::string in);
void AnalyzeCommand(std::string in);
void ExitCommand(std::string in);
void SetvalueCommand(std::string in);
