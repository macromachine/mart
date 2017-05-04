#ifndef __KLEE_SEMU_GENMU_operatorClasses__UnsignedIntegerMod__
#define __KLEE_SEMU_GENMU_operatorClasses__UnsignedIntegerMod__

/**
 * -==== UnsignedIntegerMod.h
 *
 *                LLGenMu LLVM Mutation Tool
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE.TXT for details. 
 *  
 * \brief     Matching and replacement mutation operator for Unsigned Integer MOD.
 * \details   This abstract class define is extended from the @see NumericExpression_Base class. 
 */
 
#include "../NumericArithBinop_Base.h"

class UnsignedIntegerMod: public NumericArithBinop_Base
{
  protected:
    /**
     * \brief Implement from @see NumericArithBinop_Base
     */
    inline unsigned getMyInstructionIROpCode()
    {  
        return llvm::Instruction::URem;
    }
    
  public:
    llvm::Value * createReplacement (llvm::Value * oprd1_addrOprd, llvm::Value * oprd2_intValOprd, std::vector<llvm::Value *> &replacement, ModuleUserInfos const &MI)
    {
        llvm::IRBuilder<> builder(MI.getContext());
        llvm::Value *umod = builder.CreateURem(oprd1_addrOprd, oprd2_intValOprd);
        if (!llvm::dyn_cast<llvm::Constant>(umod))
            replacement.push_back(umod);
        return umod;
    }
};

#endif //__KLEE_SEMU_GENMU_operatorClasses__UnsignedIntegerMod__
