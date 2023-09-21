/**
 * NOTE: You should feel free to manipulate any content in this .cpp file.
 * This means that if you want you can change almost everything,
 * as long as the fuzzer runs with the same cli interface.
 * This also means that if you're happy with some of the provided default
 * implementation, you don't have to modify it.
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include <cstdio>
#include <cstring>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "Utils.h"

#define ARG_EXIST_CHECK(Name, Arg)            \
  {                                           \
    struct stat Buffer;                       \
    if (stat(Arg, &Buffer)) {                 \
      fprintf(stderr, "%s not found\n", Arg); \
      return 1;                               \
    }                                         \
  }                                           \
  std::string Name(Arg);

#define DBG \
  std::cout << "Hit F::" << __FILE__ << " ::L" << __LINE__ << std::endl

/**
 * @brief Type Signature of Mutation Function.
 * MutationFn takes a string as input and returns a string.
 *
 * MutationFn: string -> string
 */
typedef std::string MutationFn(std::string);

/**
 * Struct that holds useful information about
 * one run of the program.
 *
 * @param Passed       did the program run without crashing?
 * @param Mutation     mutation function used for this run.
 * @param Input        parent input used for generating input for this run.
 * @param MutatedInput input string for this run.
 */
struct RunInfo {
  bool Passed;
  MutationFn *Mutation;
  std::string Input, MutatedInput;
};

/************************************************/
/*            Global state variables            */
/************************************************/
/**
 * Note: Feel free to add/remove/change any of the following variables.
 * Depending on what you want to keep track of during fuzzing.
 */
// Collection of strings used to generate inputs
std::vector<std::string> SeedInputs;

// Variable to store coverage related information.
std::vector<std::string> CoverageState;

// Coverage related information from previous step.
std::vector<std::string> PrevCoverageState;

/**
 * @brief Variable to keep track of some Mutation related state.
 * Feel free to change/ignore this if you want to.
 */
int MutationState[] = {1, 1, 1, 1};
std::vector<int> InputWeights;

/**
 * @brief Variable to keep track of some state related to strategy selection.
 * Feel free to change/ignore this if you want to.
 */
int StrategyState[] = {1, 2, 5, 2};
int StrategyStateSize = sizeof(StrategyState) / sizeof(StrategyState[0]);

/************************************************/
/*    Implement your select input algorithm     */
/************************************************/

/**
 * @brief Select a string that will be mutated to generate a new input.
 * Sample code picks the first input string from SeedInputs.
 *
 * TODO: Implement your logic for selecting a input to mutate.
 * If you require, you can use the Info variable to help make a
 * decision while selecting a Seed but it is not necessary for the lab.
 *
 * @param RunInfo struct with information about the previous run.
 * @return Pointer to a string.
 */
std::string selectInput(RunInfo Info) {
  int Index =
      rand() % std::accumulate(InputWeights.begin(), InputWeights.end(), 0);
  int i = 0; DBG;
  while (Index >= InputWeights[i]) Index -= InputWeights[i++];
  
  return SeedInputs[i];
}

/*********************************************/
/*       Implement mutation strategies       */
/*********************************************/

const char ALPHA[] = "abcdefghijklmnopqrstuvwxyz\n\0";
const int LENGTH_ALPHA = sizeof(ALPHA);

/**
 * Here we provide a two sample mutation functions
 * that take as input a string and returns a string.
 */

/**
 * @brief Mutation Strategy that does nothing.
 *
 * @param Original Original input string.
 * @return std::string mutated string.
 */
std::string mutationA(std::string Original) { return Original; }

/**
 * @brief Mutation Strategy that inserts a random
 * alpha numeric char at a random location in Original.
 *
 * @param Original Original input string.
 * @return std::string mutated string.
 */
std::string mutationB(std::string Original) {
  if (Original.length() <= 0) return Original;

  for (int i = 0; i < MutationState[1];) {
    int Index = rand() % Original.length();
    int Length = rand() % std::min(MutationState[1] - i, LENGTH_ALPHA) + 1;
    Original.insert(Index, Length, ALPHA[rand() % LENGTH_ALPHA]);
    i += Length;
  }

  return Original;
}

/**
 * TODO: Add your own mutation functions below.
 * Make sure to update the MutationFns vector to include your functions.
 *
 * Some ideas: swap adjacent chars, increment/decrement a char.
 *
 * Get creative with your strategies.
 */

/**
 * @brief Mutation Strategy that replaces a random
 * alpha numeric char at a random location in Original.
 *
 * @param Original Original input string.
 * @return std::string mutated string.
 */
std::string mutationC(std::string Original) {
  if (Original.length() <= 0) return Original;

  for (int i = 0; i < MutationState[2];) {
    int Index = rand() % Original.length();
    int Length = rand() % std::min(MutationState[2] - i, LENGTH_ALPHA) + 1;
    Original.replace(Index, Length, ALPHA[rand() % LENGTH_ALPHA], Length);
    i += Length;
  }

  return Original;
}

std::string mutationD(std::string Original) {
  if (Original.length() <= 0) return Original;

  for (int i = 0; i < MutationState[3] && Original.length();) {
    int Index = rand() % Original.length();
    int Length = rand() % (MutationState[3] - i) + 1;
    Original.erase(Index, Length);
    i += Length;
  }

  return Original;
}

/**
 * @brief Vector containing all the available mutation functions
 *
 * TODO: Update the definition to include any mutations you implement.
 * For example if you implement mutationC then update it to be:
 * std::vector<MutationFn *> MutationFns = {mutationA, mutationB, mutationC};
 */
std::vector<MutationFn *> MutationFns = {mutationA, mutationB, mutationC,
                                         mutationD};

/**
 * @brief Select a mutation function to apply to the seed input.
 * Sample code picks a random Strategy.
 *
 * TODO: Add your own logic to select a mutation function from MutationFns.
 * Hint: You may want to make use of any global state you store
 * during feedback to make decisions on what MutationFn to choose.
 *
 * @param RunInfo struct with information about the current run.
 * @returns a pointer to a MutationFn
 */
MutationFn *selectMutationFn(RunInfo &Info) {
  int Strat = rand() % std::accumulate(StrategyState,
                                       StrategyState + StrategyStateSize, 0);
  int i = 0;
  while (Strat >= StrategyState[i]) Strat -= StrategyState[i++];

  return MutationFns[i];
}

/*********************************************/
/*     Implement your feedback algorithm     */
/*********************************************/
/**
 * Update the internal state of the fuzzer using coverage feedback.
 *
 * @param Target name of target binary
 * @param Info RunInfo
 */
void feedBack(std::string &Target, RunInfo &Info) {
  std::vector<std::string> RawCoverageData;
  readCoverageFile(Target, RawCoverageData);

  PrevCoverageState = CoverageState;
  CoverageState.clear();

  /**
   * TODO: Implement your logic to use the coverage information from the test
   * phase to guide fuzzing. The sky is the limit!
   *
   * Hint: You want to rely on some amount of randomness to make decisions.
   *
   * You have the Coverage information of the previous test in
   * PrevCoverageState. And the raw coverage data is loaded into RawCoverageData
   * from the Target.cov file. You can either use this raw data directly or
   * process it (not-necessary). If you do some processing, make sure to update
   * CoverageState to make it available in the next call to feedback.
   */
  ptrdiff_t lastInput = std::distance(
      SeedInputs.begin(),
      std::find(SeedInputs.begin(), SeedInputs.end(), Info.Input));

  ptrdiff_t lastMutation = std::distance(
      MutationFns.begin(),
      std::find(MutationFns.begin(), MutationFns.end(), Info.Mutation));

  std::vector<std::string> diff;
  std::set_difference(RawCoverageData.begin(), RawCoverageData.end(),
                      PrevCoverageState.begin(), PrevCoverageState.end(),
                      std::back_inserter(diff));

  if ((MutationState[0] = diff.size()) > 0) {
    SeedInputs.insert(SeedInputs.end(), Info.MutatedInput);
    InputWeights.insert(InputWeights.end(), diff.size() * 8);
  }

  if (!Info.Passed) {
    StrategyState[lastMutation] += diff.size();
    InputWeights[lastInput] += diff.size() * 8;
  } else {
    if (lastMutation) MutationState[lastMutation] += 1;
    if (InputWeights[lastInput] > 1) InputWeights[lastInput] -= 1;
  }
  
  CoverageState.assign(RawCoverageData.begin(),
                       RawCoverageData.end());  // No extra processing
  // std::set_union(CoverageState.begin(), CoverageState.end(),
  //                PrevCoverageState.begin(), PrevCoverageState.end(),
  //                std::back_inserter(CoverageState));
}

int Freq = 1000;
int Count = 0;
int PassCount = 0;

bool test(std::string &Target, std::string &Input, std::string &OutDir) {
  // Clean up old coverage file before running
  std::string CoveragePath = Target + ".cov";
  std::remove(CoveragePath.c_str());

  ++Count;
  int ReturnCode = runTarget(Target, Input);
  if (ReturnCode == 127) {
    fprintf(stderr, "%s not found\n", Target.c_str());
    exit(1);
  }
  fprintf(stderr, "\e[A\rTried %d inputs, %d crashes found\n", Count,
          failureCount);
  if (ReturnCode == 0) {
    if (PassCount++ % Freq == 0) storePassingInput(Input, OutDir);
    return true;
  } else {
    storeCrashingInput(Input, OutDir);
    return false;
  }
}

/**
 * @brief Fuzz the Target program and store the results to OutDir
 *
 * @param Target Target (instrumented) program binary.
 * @param OutDir Directory to store fuzzing results.
 */
void fuzz(std::string Target, std::string OutDir) {
  struct RunInfo Info;
  while (true) {
    std::string Input = selectInput(Info);
    Info = RunInfo();
    Info.Input = Input;
    Info.Mutation = selectMutationFn(Info);
    Info.MutatedInput = Info.Mutation(Info.Input);
    Info.Passed = test(Target, Info.MutatedInput, OutDir);
    feedBack(Target, Info);
  }
}

/**
 * Usage:
 * ./fuzzer [target] [seed input dir] [output dir] [frequency] [random seed]
 */
int main(int argc, char **argv) {
  if (argc < 4) {
    printf(
        "usage %s [target] [seed input dir] [output dir] [frequency "
        "(optional)] [seed (optional arg)]\n",
        argv[0]);
    return 1;
  }

  ARG_EXIST_CHECK(Target, argv[1]);
  ARG_EXIST_CHECK(SeedInputDir, argv[2]);
  ARG_EXIST_CHECK(OutDir, argv[3]);

  if (argc >= 5) Freq = strtol(argv[4], NULL, 10);

  int RandomSeed = argc > 5 ? strtol(argv[5], NULL, 10) : (int)time(NULL);

  srand(RandomSeed);
  storeSeed(OutDir, RandomSeed);
  initialize(OutDir);

  if (readSeedInputs(SeedInputs, SeedInputDir)) {
    fprintf(stderr, "Cannot read seed input directory\n");
    return 1;
  }

  InputWeights = std::vector<int>(SeedInputs.size(), 32);

  fprintf(stderr, "Fuzzing %s...\n\n", Target.c_str());
  fuzz(Target, OutDir);
  return 0;
}