#ifndef __KLEE_SEMU_GENMU_operatorClasses__BitOr__
#define __KLEE_SEMU_GENMU_operatorClasses__BitOr__

/**
 * -==== BitOr.h
 *
 *                LLGenMu LLVM Mutation Tool
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE.TXT for details. 
 *  
 * \brief     Matching and replacement mutation operator for BIT OR.
 * \details   This abstract class define is extended from the @see NumericExpression_Base class. 
 */
 
#include "../NumericArithBinop_Base.h"

class BitOr: public NumericArithBinop_Base
{
  protected:
    /**
     * \brief Implement from @see NumericArithBinop_Base
     */
    inline unsigned getMyInstructionIROpCode()
    {  
        return llvm::Instruction::Or;
    }
    
  public:
    llvm::Value * createReplacement (llvm::Value * oprd1_addrOprd, llvm::Value * oprd2_intValOprd, std::vector<llvm::Value *> &replacement, ModuleUserInfos const &MI)
    {
        llvm::IRBuilder<> builder(MI.getContext());
        llvm::Value *bor = builder.CreateOr(oprd1_addrOprd, oprd2_intValOprd);
        if (bor != oprd1_addrOprd && bor != oprd2_intValOprd)
            if (!llvm::dyn_cast<llvm::Constant>(bor))
                replacement.push_back(bor);
        return bor;
    }
};

#endif //__KLEE_SEMU_GENMU_operatorClasses__BitOr__
