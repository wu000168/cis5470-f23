#include "DivZeroAnalysis.h"
#include "Utils.h"

namespace dataflow {
using namespace llvm;
/**
 * @brief Is the given instruction a user input?
 *
 * @param Inst The instruction to check.
 * @return true If it is a user input, false otherwise.
 */
bool isInput(Instruction *Inst) {
  if (auto Call = dyn_cast<CallInst>(Inst)) {
    if (auto Fun = Call->getCalledFunction()) {
      return (Fun->getName().equals("getchar") ||
              Fun->getName().equals("fgetc"));
    }
  }
  return false;
}

/**
 * Evaluate a PHINode to get its Domain.
 *
 * @param Phi PHINode to evaluate
 * @param InMem InMemory of Phi
 * @return Domain of Phi
 */
Domain *eval(PHINode *Phi, const Memory *InMem) {
  if (auto ConstantVal = Phi->hasConstantValue()) {
    return new Domain(extractFromValue(ConstantVal));
  }

  Domain *Joined = new Domain(Domain::Uninit);

  for (unsigned int i = 0; i < Phi->getNumIncomingValues(); i++) {
    auto Dom = getOrExtract(InMem, Phi->getIncomingValue(i));
    Joined = Domain::join(Joined, Dom);
  }
  return Joined;
}

/**
 * @brief Evaluate the +, -, * and / BinaryOperator instructions
 * using the Domain of its operands and return the Domain of the result.
 *
 * @param BinOp BinaryOperator to evaluate
 * @param InMem InMemory of BinOp
 * @return Domain of BinOp
 */
Domain *eval(BinaryOperator *BinOp, const Memory *InMem) {
  /**
   * TODO: Write your code here that evaluates +, -, * and /
   * based on the Domains of the operands.
   */
  AddrSpaceCastInst::BinaryOps opCode = BinOp->getOpcode();
  Domain *dom1 = getOrExtract(InMem, BinOp->getOperand(0));
  Domain *dom2 = getOrExtract(InMem, BinOp->getOperand(1));
  switch (opCode) {
    case AddrSpaceCastInst::BinaryOps::Add:
      return Domain::add(dom1, dom2);
    case AddrSpaceCastInst::BinaryOps::Sub:
      return Domain::sub(dom1, dom2);
    case AddrSpaceCastInst::BinaryOps::Mul:
      return Domain::mul(dom1, dom2);
    case AddrSpaceCastInst::BinaryOps::UDiv:
    case AddrSpaceCastInst::BinaryOps::SDiv:
      return Domain::div(dom1, dom2);
    default:
      return new Domain(Domain::MaybeZero);
  }
}

/**
 * @brief Evaluate Cast instructions.
 *
 * @param Cast Cast instruction to evaluate
 * @param InMem InMemory of Instruction
 * @return Domain of Cast
 */
Domain *eval(CastInst *Cast, const Memory *InMem) {
  /**
   * TODO: Write your code here to evaluate Cast instruction.
   */
  return getOrExtract(InMem, Cast->getOperand(0));
}

/**
 * @brief Evaluate the ==, !=, <, <=, >=, and > Comparision operators using
 * the Domain of its operands to compute the Domain of the result.
 *
 * @param Cmp Comparision instruction to evaluate
 * @param InMem InMemory of Cmp
 * @return Domain of Cmp
 */
Domain *eval(CmpInst *Cmp, const Memory *InMem) {
  /**
   * TODO: Write your code here that evaluates:
   * ==, !=, <, <=, >=, and > based on the Domains of the operands.
   *
   * NOTE: There is a lot of scope for refining this, but you can just return
   * MaybeZero for comparisons other than equality.
   */
  CmpInst::Predicate predicate = Cmp->getPredicate();
  Domain *E1 = getOrExtract(InMem, Cmp->getOperand(0));
  Domain *E2 = getOrExtract(InMem, Cmp->getOperand(1));
  if (E1->Value == Domain::Uninit || E2->Value == Domain::Uninit)
    return new Domain(Domain::Uninit);
  switch (predicate) {
    case CmpInst::Predicate::ICMP_EQ:
      if (E1->Value == Domain::Zero && E2->Value == Domain::Zero)
        return new Domain(Domain::NonZero);
      if (E1->Value == Domain::Zero && E2->Value == Domain::NonZero ||
          E1->Value == Domain::NonZero && E2->Value == Domain::Zero)
        return new Domain(Domain::Zero);
      return new Domain(Domain::MaybeZero);
    case CmpInst::Predicate::ICMP_NE:
      if (E1->Value == Domain::Zero && E2->Value == Domain::Zero)
        return new Domain(Domain::Zero);
      if (E1->Value == Domain::Zero && E2->Value == Domain::NonZero ||
          E1->Value == Domain::NonZero && E2->Value == Domain::Zero)
        return new Domain(Domain::NonZero);
      return new Domain(Domain::MaybeZero);
    case CmpInst::Predicate::ICMP_UGT:
    case CmpInst::Predicate::ICMP_SGT:
    case CmpInst::Predicate::ICMP_ULT:
    case CmpInst::Predicate::ICMP_SLT:
      if (E1->Value == Domain::Zero && E2->Value == Domain::Zero)
        return new Domain(Domain::Zero);
      return new Domain(Domain::MaybeZero);
    case CmpInst::Predicate::ICMP_UGE:
    case CmpInst::Predicate::ICMP_SGE:
    case CmpInst::Predicate::ICMP_ULE:
    case CmpInst::Predicate::ICMP_SLE:
      if (E1->Value == Domain::Zero && E2->Value == Domain::Zero)
        return new Domain(Domain::NonZero);
      return new Domain(Domain::MaybeZero);
    default:
      return new Domain(Domain::MaybeZero);
  }
}

void DivZeroAnalysis::transfer(Instruction *Inst, const Memory *In,
                               Memory &NOut) {
  if (isInput(Inst)) {
    // The instruction is a user controlled input, it can have any value.
    NOut[variable(Inst)] = new Domain(Domain::MaybeZero);
  } else if (auto Phi = dyn_cast<PHINode>(Inst)) {
    // Evaluate PHI node
    NOut[variable(Phi)] = eval(Phi, In);
  } else if (auto BinOp = dyn_cast<BinaryOperator>(Inst)) {
    // Evaluate BinaryOperator
    NOut[variable(BinOp)] = eval(BinOp, In);
  } else if (auto Cast = dyn_cast<CastInst>(Inst)) {
    // Evaluate Cast instruction
    NOut[variable(Cast)] = eval(Cast, In);
  } else if (auto Cmp = dyn_cast<CmpInst>(Inst)) {
    // Evaluate Comparision instruction
    NOut[variable(Cmp)] = eval(Cmp, In);
  } else if (auto Alloca = dyn_cast<AllocaInst>(Inst)) {
    // Used for the next lab, do nothing here.
  } else if (auto Store = dyn_cast<StoreInst>(Inst)) {
    // Used for the next lab, do nothing here.
  } else if (auto Load = dyn_cast<LoadInst>(Inst)) {
    // Used for the next lab, do nothing here.
  } else if (auto Branch = dyn_cast<BranchInst>(Inst)) {
    // Analysis is flow-insensitive, so do nothing here.
  } else if (auto Call = dyn_cast<CallInst>(Inst)) {
    // Analysis is intra-procedural, so do nothing here.
  } else if (auto Return = dyn_cast<ReturnInst>(Inst)) {
    // Analysis is intra-procedural, so do nothing here.
  } else {
    errs() << "Unhandled instruction: " << *Inst << "\n";
  }
}

}  // namespace dataflow