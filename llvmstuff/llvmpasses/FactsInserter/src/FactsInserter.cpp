//
// Created by machiry at the beginning of time.
//

#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <iostream>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Support/Debug.h>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include "FactsSelector.h"
#include "ModuleHelper.h"
#include "FactsInserterManager.h"
#include "ReverseCallGraph.h"


using namespace llvm;

namespace UtilPasses {


    static cl::opt<std::string> outputFile("outputFile",
                                           cl::desc("Path to the output file, where all the warnings should be stored."),
                                           cl::value_desc("Path of the output file."), cl::init("target_ou.json"));

    static cl::opt<std::string> NUMFACTS("numFacts",
                                            cl::desc("Total number of facts to be inserted."),
                                            cl::value_desc("Unsigned long value indicating total number of facts to be inserted."));

    static cl::bits<FACTTYPE> TARGETFACTTYPE(cl::desc("Select fact type to be inserted"),
                                                    cl::values(
                                                            clEnumValN(FACTTYPE::PTRFACT, "dataptr", "Data pointer facts."),
                                                            clEnumValN(FACTTYPE::FUNCPTRFACT, "funcptr", "Function pointer facts.")
                                                    ));

    static cl::bits<FACTSENSITIVITY> TARGETSENSITIVITY(cl::desc("Select sensitivity of the facts to be inserted."),
                                             cl::values(
                                                     clEnumValN(FACTSENSITIVITY::NOSENSITIVITY, "ns", "Insensitive"),
                                                     clEnumValN(FACTSENSITIVITY::FLOWSENSITIVE, "fs", "Flow sensitive."),
                                                     clEnumValN(FACTSENSITIVITY::PATHSENSITIVE, "ps", "Path sensitive."),
                                                     clEnumValN(FACTSENSITIVITY::CONTEXTSENSITIVE, "cs", "Context sensitive."),
                                                     clEnumValN(FACTSENSITIVITY::FLOWPATHSENSITIVE, "fps", "Flow and path sensitive."),
                                                     clEnumValN(FACTSENSITIVITY::FLOWCONTEXTSENSITIVE, "fcs", "Flow and context sensitive.")


                                             ));



    /***
     * The main pass.
     */
    struct InsertFacts: public ModulePass {
    public:
        static char ID;

        unsigned long totalFacts;

        std::map<Function*, LoopInfo*> funcLoopInfo;

        InsertFacts() : ModulePass(ID) {
            std::string::size_type sz;
            totalFacts = std::stol(NUMFACTS,&sz);
            // initialize the selector.
            FactsSelector::initializeSelector();
        }

        ~InsertFacts() {
        }

        void generateLoopInfo(Module &m) {
            this->funcLoopInfo.clear();
            for(auto &currF:m.getFunctionList()) {
                if (!currF.isDeclaration()) {
                    LoopInfo &currLInfo = getAnalysis<LoopInfoWrapperPass>(currF).getLoopInfo();
                    this->funcLoopInfo[&currF] = &currLInfo;
                }
            }
        }


        bool runOnModule(Module &m) override {

            // main processing loop
            unsigned long numInsertedFacts = 0;
            bool factTypeConfigured = false, sensitivityConfigured = false;

            CallGraph &currCG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

            this->generateLoopInfo(m);
            ReverseCallGraph *currRev = new ReverseCallGraph(m, currCG);

            ModuleHelper *currModHelper = new ModuleHelper(m, currRev, this->funcLoopInfo);

            FactsInserterManager *currInsertManager = new FactsInserterManager(m, currModHelper);

            FACTTYPE currFactType = FACTTYPE::MAXFACTTYPE;
            for (unsigned i = 0; i< FACTTYPE::MAXFACTTYPE; i++) {
                if (TARGETFACTTYPE.isSet(i)){
                    currFactType = (FACTTYPE)i;
                    factTypeConfigured = true;
                    break;
                }
            }


            FACTSENSITIVITY currFactSens = FACTSENSITIVITY::MAXSENSITIVITY;
            for (unsigned i = 0; i< FACTSENSITIVITY::MAXSENSITIVITY; i++) {
                if (TARGETSENSITIVITY.isSet(i)){
                    currFactSens = (FACTSENSITIVITY)i;
                    sensitivityConfigured = true;
                    break;
                }
            }


            while(numInsertedFacts < totalFacts) {
                // get the type of the fact to be inserted.
                if(!factTypeConfigured) {
                    currFactType = FactsSelector::getNextFactType();
                }
                // get the sensitivity of the next fact.
                if(!sensitivityConfigured) {
                    currFactSens = FactsSelector::getNextFactSensitivity();
                }

                // sanity
                assert(currFactType < FACTTYPE::MAXFACTTYPE);
                assert(currFactSens < FACTSENSITIVITY::MAXSENSITIVITY);

                switch(currFactType) {
                    case FACTTYPE::FUNCPTRFACT:
                        if(currInsertManager->insertFuncPtrFact(currFactSens)) {
                            numInsertedFacts++;
                            DEBUG_PRINT("Inserted Function Pointer fact for:%lu\n", numInsertedFacts);
                        } else {
                            ERROR_PRINT("Error occurred while trying to insert function pointer fact for:%lu\n", numInsertedFacts);
                        }
                        break;
                    case FACTTYPE::PTRFACT:
                        if(currInsertManager->insertDataPtrFact(currFactSens)) {
                            numInsertedFacts++;
                            DEBUG_PRINT("Inserted Data Pointer fact for:%lu\n", numInsertedFacts);
                        } else {
                            ERROR_PRINT("Error occurred while trying to insert data pointer fact for:%lu\n", numInsertedFacts);
                        }
                        break;
                    default: assert("Unreachable. We should never reach here."); break;
                }
            }

            std::error_code res_code;
            llvm::raw_fd_ostream output_stream(outputFile, res_code, llvm::sys::fs::F_Text);

            dbgs() << "[+] Dumping results to output:" << outputFile << "\n";

            output_stream << "{\"total_time\":" << currInsertManager->getTotalTime() << ",\n";
            output_stream << "\"total_path\":" << currInsertManager->totalPathLength << ",\n";
            output_stream << "\"total_facts\":" << numInsertedFacts << ",\n";
            output_stream << "\"fact_type\":" << TARGETFACTTYPE.getBits() << ",\n";
            output_stream << "\"sensitivity\":" << TARGETSENSITIVITY.getBits() << "}";

            output_stream.close();

            return true;
        }

        void getAnalysisUsage(AnalysisUsage &AU) const override {
            AU.addRequired<CallGraphWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
        }

    };

    char InsertFacts::ID = 0;
    static RegisterPass<InsertFacts> x("insfacts", "Auto facts inserter.", false, false);
}
