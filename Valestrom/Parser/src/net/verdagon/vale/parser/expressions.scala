package net.verdagon.vale.parser

import net.verdagon.vale.vassert

trait IExpressionPE

case class VoidPE(range: Range) extends IExpressionPE {}

case class LendPE(range: Range, expr: IExpressionPE, targetOwnership: OwnershipP) extends IExpressionPE {
  targetOwnership match {
    case WeakP =>
    case BorrowP =>
  }
}

case class AndPE(left: IExpressionPE, right: IExpressionPE) extends IExpressionPE

case class OrPE(left: IExpressionPE, right: IExpressionPE) extends IExpressionPE

case class MutablePE(expr: IExpressionPE) extends IExpressionPE

case class IfPE(range: Range, condition: BlockPE, thenBody: BlockPE, elseBody: BlockPE) extends IExpressionPE
case class WhilePE(range: Range, condition: BlockPE, body: BlockPE) extends IExpressionPE
case class DestructPE(range: Range, inner: IExpressionPE) extends IExpressionPE
case class MatchPE(range: Range, condition: IExpressionPE, lambdas: List[LambdaPE]) extends IExpressionPE
case class MutatePE(range: Range, mutatee: IExpressionPE, expr: IExpressionPE) extends IExpressionPE
case class ReturnPE(range: Range, expr: IExpressionPE) extends IExpressionPE
case class SwapPE(exprA: IExpressionPE, exprB: IExpressionPE) extends IExpressionPE

case class LetPE(
  range: Range,
    templateRules: List[IRulexPR],
    pattern: PatternPP,
    expr: IExpressionPE
) extends IExpressionPE

case class RepeaterBlockPE(expression: IExpressionPE) extends IExpressionPE

case class RepeaterBlockIteratorPE(expression: IExpressionPE) extends IExpressionPE
case class SequencePE(range: Range, elements: List[IExpressionPE]) extends IExpressionPE

case class RepeaterPackPE(expression: IExpressionPE) extends IExpressionPE
case class RepeaterPackIteratorPE(expression: IExpressionPE) extends IExpressionPE

case class IntLiteralPE(range: Range, value: Int) extends IExpressionPE
case class BoolLiteralPE(range: Range, value: Boolean) extends IExpressionPE
case class StrLiteralPE(value: StringP) extends IExpressionPE
case class FloatLiteralPE(range: Range, value: Float) extends IExpressionPE

case class DotPE(
  range: Range,
  left: IExpressionPE,
  operatorRange: Range,
  isMapAccess: Boolean,
  member: LookupPE) extends IExpressionPE

case class IndexPE(range: Range, left: IExpressionPE, args: List[IExpressionPE]) extends IExpressionPE

case class FunctionCallPE(
  range: Range,
  inline: Option[UnitP],
  operatorRange: Range,
  isMapCall: Boolean,
  callableExpr: IExpressionPE,
  argExprs: List[IExpressionPE],
  // If we're calling a lambda or some other callable struct,
  // the 'this' ptr parameter might want a certain ownership,
  // so the user might specify that.
  targetOwnershipForCallable: OwnershipP
) extends IExpressionPE

case class MethodCallPE(
  range: Range,
  callableExpr: IExpressionPE,
  operatorRange: Range,
  callableTargetOwnership: OwnershipP,
  isMapCall: Boolean,
  methodLookup: LookupPE,
  argExprs: List[IExpressionPE]
) extends IExpressionPE

case class TemplateArgsP(range: Range, args: List[ITemplexPT])
case class LookupPE(name: StringP, templateArgs: Option[TemplateArgsP]) extends IExpressionPE
case class MagicParamLookupPE(range: Range) extends IExpressionPE

case class LambdaPE(
  // Just here for syntax highlighting so far
  captures: Option[UnitP],
  function: FunctionP
) extends IExpressionPE

case class BlockPE(range: Range, elements: List[IExpressionPE]) extends IExpressionPE {
  // Every element should have at least one expression, because a block will
  // return the last expression's result as its result.
  // Even empty blocks aren't empty, they have a void() at the end.
  vassert(elements.size >= 1)
}
