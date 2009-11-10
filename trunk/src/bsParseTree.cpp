#include <sstream>
#include <iostream>
#include "bsParseTree.h"
#include "bsScriptMachine.h"
#include "bsFireType.h"

BS_NMSP::ScriptMachine* BS_NMSP::ParseTree::mScriptMachine = 0;

BS_NMSP::ParseTree* BS_NMSP::ParseTree::msInstance = 0;

namespace BS_NMSP
{

// --------------------------------------------------------------------------------
ParseTreeNode::ParseTreeNode(int type, int line, ScriptMachine* scriptMachine,
							 ParseTree* tree) :
	mTree(tree),
	mParent(0),
	mScriptMachine(scriptMachine),
	mType(type),
	mLine(line)
{
	for (int i = 0; i < MAX_CHILDREN; ++i)
		mChildren[i] = 0;
}
// --------------------------------------------------------------------------------
ParseTreeNode::~ParseTreeNode()
{
	for (int i = 0; i < MAX_CHILDREN; ++i)
		delete mChildren[i];
}
// --------------------------------------------------------------------------------
void ParseTreeNode::_setType(int type)
{
	mType = type;
}
// --------------------------------------------------------------------------------
void ParseTreeNode::setChild(int index, ParseTreeNode *node)
{
	mChildren[index] = node;
	if (mChildren[index])
		mChildren[index]->mParent = this;
}
// --------------------------------------------------------------------------------
ParseTreeNode* ParseTreeNode::getChild(int index) const
{
	return mChildren[index];
}
// --------------------------------------------------------------------------------
ParseTreeNode* ParseTreeNode::getParent() const
{
	return mParent;
}
// --------------------------------------------------------------------------------
ParseTree* ParseTreeNode::getTree() const
{
	return mTree;
}
// --------------------------------------------------------------------------------
int ParseTreeNode::getType() const
{
	return mType;
}
// --------------------------------------------------------------------------------
int ParseTreeNode::getLine() const
{
	return mLine;
}
// --------------------------------------------------------------------------------
void ParseTreeNode::foldBinaryNode()
{
	if (mChildren[0]->getType() != PT_Constant ||
		mChildren[1]->getType() != PT_Constant)
	{
		return;
	}

	bstype val1 = mChildren[0]->getValueData();
	bstype val2 = mChildren[1]->getValueData();

	switch (mType)
	{
	case PT_AddStatement:
		setValue(val1 + val2);
		break;

	case PT_SubtractStatement:
		setValue(val1 - val2);
		break;

	case PT_MultiplyStatement:
		setValue(val1 * val2);
		break;

	case PT_DivideStatement:
		setValue(val1 / val2);
		break;

	case PT_RemainderStatement:
		setValue((bstype) ((int) val1 % (int) val2));
		break;

	case PT_EqualsStatement:
		setValue(val1 == val2 ? bsvalue1 : bsvalue0);
		break;

	case PT_NotEqualsStatement:
		setValue(val1 != val2 ? bsvalue1 : bsvalue0);
		break;

	case PT_LessThanStatement:
		setValue(val1 < val2 ? bsvalue1 : bsvalue0);
		break;

	case PT_GreaterThanStatement:
		setValue(val1 > val2 ? bsvalue1 : bsvalue0);
		break;

	case PT_LessThanEqStatement:
		setValue(val1 <= val2 ? bsvalue1 : bsvalue0);
		break;

	case PT_GreaterThanEqStatement:
		setValue(val1 >= val2 ? bsvalue1 : bsvalue0);
		break;
	}

	mType = PT_Constant;
	for (int i = 0; i < MAX_CHILDREN; ++i)
	{
		if (mChildren[i])
		{
			delete mChildren[i];
			mChildren[i] = 0;
		}
	}
}
// --------------------------------------------------------------------------------
void ParseTreeNode::foldUnaryNode ()
{
	if (mChildren[0]->getType () != PT_Constant)
	{
		return;
	}

	bstype val = mChildren[0]->getValueData();

	switch (mType)
	{
	case PT_UnaryPosStatement:
		setValue(val);
		break;

	case PT_UnaryNegStatement:
		setValue(-val);
		break;

	case PT_ConstantExpression:
		setValue(val);
		break;
	}

	mType = PT_Constant;
	for (int i = 0; i < MAX_CHILDREN; ++i)
	{
		if (mChildren[i])
		{
			delete mChildren[i];
			mChildren[i] = 0;
		}
	}
}
// --------------------------------------------------------------------------------
void ParseTreeNode::foldLogicalNode ()
{
	if (mChildren[0]->getType () != PT_Constant ||
		mChildren[1]->getType () != PT_Constant)
	{
		return;
	}

	bstype val1 = mChildren[0]->getValueData();
	bstype val2 = mChildren[1]->getValueData();

	switch (mType)
	{
	case PT_LogicalOr:
		setValue(val1 || val2 ? bsvalue1 : bsvalue0);
		break;

	case PT_LogicalAnd:
		setValue(val1 && val2 ? bsvalue1 : bsvalue0);
		break;
	}

	mType = PT_Constant;
	for (int i = 0; i < MAX_CHILDREN; ++i)
	{
		if (mChildren[i])
		{
			delete mChildren[i];
			mChildren[i] = 0;
		}
	}
}
// --------------------------------------------------------------------------------
void ParseTreeNode::foldConstants()
{
	for (int i = 0; i < MAX_CHILDREN; ++i)
	{
		if (mChildren[i])
		{
			mChildren[i]->foldConstants();
		}
	}

	switch (mType)
	{
	case PT_AddStatement:
	case PT_SubtractStatement:
	case PT_MultiplyStatement:
	case PT_DivideStatement:
	case PT_RemainderStatement:
	case PT_EqualsStatement:
	case PT_NotEqualsStatement:
	case PT_LessThanStatement:
	case PT_GreaterThanStatement:
	case PT_LessThanEqStatement:
	case PT_GreaterThanEqStatement:
		foldBinaryNode();
		break;

	case PT_UnaryPosStatement:
	case PT_UnaryNegStatement:
	case PT_ConstantExpression:
		foldUnaryNode();
		break;

	case PT_LogicalOr:
	case PT_LogicalAnd:
		foldLogicalNode();
		break;

	default:
		break;
	}
}
// --------------------------------------------------------------------------------
void ParseTreeNode::setValue(bstype data)
{
	mDataType = DT_Value;
	mValueData = data;
}
// --------------------------------------------------------------------------------
void ParseTreeNode::setString(const char* data)
{
	mDataType = DT_String;
	mStringData = data;
}
// --------------------------------------------------------------------------------
int ParseTreeNode::getDataType() const
{
	return mDataType;
}
// --------------------------------------------------------------------------------
bstype ParseTreeNode::getValueData() const
{
	return mValueData;
}
// --------------------------------------------------------------------------------
const String& ParseTreeNode::getStringData() const
{
	return mStringData;
}
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
ParseTree::ParseTree()
{
	create();
}
// --------------------------------------------------------------------------------
ParseTree::~ParseTree()
{
	destroy();
}
// --------------------------------------------------------------------------------
void ParseTree::create()
{
	mRoot = new ParseTreeNode(PT_Null, 0, mScriptMachine, this);
	mNumErrors = 0;
}
// --------------------------------------------------------------------------------
void ParseTree::destroy()
{
	mCodeblockNames.clear();
	delete mRoot;
	mRoot = 0;
}
// --------------------------------------------------------------------------------
void ParseTree::reset()
{
	destroy();
	create();
}
// --------------------------------------------------------------------------------
ParseTree *ParseTree::instancePtr()
{
	if (!msInstance)
	{
		msInstance = new ParseTree;
	}

	return msInstance;
}
// --------------------------------------------------------------------------------
void ParseTree::setMachines(ScriptMachine *scriptMachine)
{
	mScriptMachine = scriptMachine;
}
// --------------------------------------------------------------------------------
ParseTreeNode* ParseTree::getRootNode()
{
	return mRoot;
}
// --------------------------------------------------------------------------------
ParseTreeNode* ParseTree::createNode(int type, int line)
{
	return new ParseTreeNode(type, line, mScriptMachine, this);
}
// --------------------------------------------------------------------------------
void ParseTree::foldConstants()
{
	mRoot->foldConstants();
}
// --------------------------------------------------------------------------------
void ParseTree::addError(int line, const String& msg)
{
	std::stringstream ss;
	ss << "[" << line << "] " << msg;
	
	mScriptMachine->addErrorMsg(ss.str());
	mNumErrors++;
}
// --------------------------------------------------------------------------------
int ParseTree::getNumErrors() const
{
	return mNumErrors;
}
// --------------------------------------------------------------------------------
bool ParseTree::checkConstantExpression(EmitterDefinition* def, ParseTreeNode* node)
{
	int nodeType = node->getType();
	if (nodeType == PT_FunctionCall)
	{
		String funcName = node->getChild(0)->getStringData();
		int index = mScriptMachine->getNativeFunctionIndex(funcName);
		if (index < 0)
		{
			addError(node->getLine(), "Function '" + funcName + "' not found.");
			return false;
		}
		else
		{
			return true;
		}
	}
	else if (nodeType == PT_Identifier)
	{
		String varName = node->getStringData();

		// Check local, member, global
		int numStates = def->getNumStates();
		EmitterDefinition::State& st = def->getState(numStates - 1);
		CodeRecord* rec = getCodeRecord("Emitter", def->getName(), "State", st.name);
		if (rec->getVariableIndex(varName) < 0)
		{
			if (def->getMemberVariableIndex(varName) < 0)
			{
				if (mScriptMachine->getGlobalVariableIndex(varName) < 0)
				{
					addError(node->getLine(), "Variable '" + varName + "' is not declared.");
					return false;
				}
			}
		}
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		ParseTreeNode* child = node->getChild(i);
		if (child)
		{
			if (!checkConstantExpression(def, child))
				return false;
		}
	}

	return true;
}
// --------------------------------------------------------------------------------
void ParseTree::checkLoopDepth(ParseTreeNode *node, int& depth)
{
	int nodeType = node->getType();
	if (nodeType == PT_LoopStatement)
	{
		depth++;
		if (depth > BS_SCRIPT_LOOP_DEPTH)
		{
			std::stringstream ss;
			ss << "Loops are nested too deeply (max " << BS_SCRIPT_LOOP_DEPTH << ")";
			addError(node->getLine(), ss.str());
			return;
		}

		if (node->getChild(1))
			checkLoopDepth(node->getChild(1), depth);
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++i)
	{
		if (node->getChild(i))
			checkLoopDepth(node->getChild(i), depth);
	}

	if (nodeType == PT_LoopStatement)
		--depth;
}
// --------------------------------------------------------------------------------
void ParseTree::createEmitterMemberVariables(EmitterDefinition* def, ParseTreeNode* node)
{
	if (node->getType() == PT_AssignStatement)
	{
		String varName = node->getChild(0)->getStringData();

		// Make sure that constant expressions are valid, ie use declared variables/functions
		if (node->getChild(1)->getType() == PT_ConstantExpression)
		{
			if (!checkConstantExpression(def, node->getChild(1)))
				return;
		}

		// Make sure it doesn't already exist
		if (def->getMemberVariableIndex(varName) >= 0)
			addError(node->getLine(), "Member variable '" + varName + "' already declared.");

		def->addMemberVariable(varName, false);
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		ParseTreeNode* child = node->getChild(i);
		if (child)
			createEmitterMemberVariables(def, child);
	}
}
// --------------------------------------------------------------------------------
void ParseTree::addEmitterMemberVariables(EmitterDefinition* def, 
										  const MemberVariableDeclarationMap& memberDecls)
{
	// Add special members
	def->addMemberVariable("This_X", true);
	def->addMemberVariable("This_Y", true);
	def->addMemberVariable("This_Angle", true);

	// Add user-specified member variables (and their initial value) here.
	// They must be added before we compile the script, predeclared essentially.
	typedef MemberVariableDeclarationMap::const_iterator declIt;
	std::pair<declIt, declIt> range = memberDecls.equal_range(def->getName());

	int members = 0;
	while (range.first != range.second)
	{
		String varName = range.first->second.name;
		if (def->getMemberVariableIndex(varName) >= 0)
		{
			String msg = "Member variable '" + varName + "' already declared.";
			addError(0, msg);
		}
		else
		{
			def->addMemberVariable(varName, true, range.first->second.value);
			members++;
		}

		range.first++;
	}

	def->setNumUserMembers(members);
}
// --------------------------------------------------------------------------------
void ParseTree::addControllerMemberVariables(ControllerDefinition* def)
{
	// Add special members
	def->addMemberVariable("This_X", true);
	def->addMemberVariable("This_Y", true);
	def->addMemberVariable("This_Angle", true);
}
// --------------------------------------------------------------------------------
bool ParseTree::checkAffectorArguments(EmitterDefinition* def, ParseTreeNode* node)
{
	bool ok = true;
	if (node->getType() == PT_Identifier)
	{
		int pType = node->getParent()->getType();
		String varName = node->getStringData();
		
		if (pType >= PT_ConstantExpression && pType <= PT_UnaryNegStatement)
		{
			if (def->getMemberVariableIndex(varName) < 0)
			{
				if (mScriptMachine->getGlobalVariableIndex(varName) < 0)
				{
					addError(node->getLine(), "Variable '" + varName + "' is not declared.");
					ok = false;
				}
			}
			else
			{
				addError(node->getLine(), "Cannot use member variables in affector arguments.");
				ok = false;
			}
		}
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
		{
			if (!checkAffectorArguments(def, node->getChild(i)))
				ok = false;
		}
	}

	return ok;
}
// --------------------------------------------------------------------------------
void ParseTree::createAffectors(EmitterDefinition* def, ParseTreeNode* node)
{
	if (node->getType() == PT_AffectorDecl)
	{
		// Create temporary structure with the node.  We don't actually want to
		// create any instances yet because we don't know which FireTypes are
		// going to use them.
		String affName = node->getChild(0)->getStringData();
		for (size_t i = 0; i < mAffectors.size(); ++i)
		{
			if (mAffectors[i].name == affName)
			{
				addError(node->getLine(), "Affector '" + affName + "' already declared.");
				return;
			}
		}

		if(checkAffectorArguments(def, node->getChild(1)))
		{
			AffectorInfo info;
			info.name = affName;
			info.function = node->getChild(1)->getChild(0)->getStringData();
			countFunctionCallArguments(node->getChild(1), info.numArgs);
			info.node = node;

			mAffectors.push_back(info);
		}
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			createAffectors(def, node->getChild(i));
	}
}
// --------------------------------------------------------------------------------
void ParseTree::createEmitterVariables(ControllerDefinition* def, ParseTreeNode* node)
{
	if (node->getType() == PT_Emitter)
	{
		String varName = node->getChild(0)->getStringData();
		String emitType = node->getChild(1)->getStringData();
		
		// Make sure that a) the variable name doesn't exist, and that b) the emitter name does
		if (def->getEmitterVariableIndex(varName) >= 0)
		{
			addError(node->getLine(), "Variable '" + varName + "' already declared.");
			return;
		}
		
		if (!mScriptMachine->getEmitterDefinition(emitType))
		{
			addError(node->getLine(), "Emitter '" + emitType + "' not found.");
			return;
		}

		def->addEmitterVariable(varName, emitType);
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			createEmitterVariables(def, node->getChild(i));
	}
}
// --------------------------------------------------------------------------------
void ParseTree::countFunctionCallArguments(ParseTreeNode* node, int& numArguments)
{
	int nodeType = node->getType();
	if (nodeType == PT_FunctionCall)
		return;

	if (nodeType == PT_FunctionCallArg)
		numArguments++;

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			countFunctionCallArguments(node->getChild(i), numArguments);
	}
}
// --------------------------------------------------------------------------------
void ParseTree::checkFireControllers(EmitterDefinition* def, ParseTreeNode* node, 
									 int& ctrls, FireType* ft)
{
	int nodeType = node->getType();
	if (nodeType == PT_FunctionCall)
	{
		String ctrlName = node->getChild(0)->getStringData();
		
		// Make sure ctrlName exists...
		int fIndex = def->getFunctionIndex(ctrlName);
		if (fIndex < 0)
		{
			addError(node->getLine(), "Function '" + ctrlName + "' is not declared.");
			return;
		}

		if (ctrls > 0)
		{
			addError(node->getLine(), "Fire functions can only take one control function.");
			return;
		}
		
		ctrls++;

		// ...and that we're passing the correct number of arguments to it
		int numArguments = 0;
		if (node->getChild(1))
			countFunctionCallArguments(node->getChild(1), numArguments);

		EmitterDefinition::Function& func = def->getFunction(fIndex);
		if (numArguments != func.numArguments)
		{
			std::stringstream ss;
			ss << "Function '" << ctrlName << "' expects " << func.numArguments << " arguments.";
			addError(node->getLine(), ss.str());
			return;
		}
	}
	else if (nodeType == PT_AffectorCall)
	{
		// Named affectors
		String affector = node->getStringData();

		// Make sure affector does not take member variables as arguments
		// ...

		int index = -1;
		for (size_t i = 0; i < mAffectors.size(); ++i)
		{
			if (mAffectors[i].name == affector)
			{
				index = (int) i;
				break;
			}
		}

		if (index < 0)
		{
			addError(node->getLine(), "Affector '" + affector + "' is not declared.");
			return;
		}

		if (!ft->affectorFunctionExists(mAffectors[index].function))
		{
			addError(node->getLine(), "Affector function '" + mAffectors[index].function + "' is not"
				" registered with type '" + ft->getName() + "'.");
			return;
		}
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			checkFireControllers(def, node->getChild(i), ctrls, ft);
	}
}
// --------------------------------------------------------------------------------
void ParseTree::addFunctionArguments(EmitterDefinition* def, ParseTreeNode* node, 
									 EmitterDefinition::Function& func)
{
	if (node->getType() == PT_Identifier)
	{
		String argName = node->getStringData();

		CodeRecord* rec = getCodeRecord("Emitter", def->getName(), "Function", func.name);
		if (rec->getVariableIndex(argName) >= 0)
		{
			addError(node->getLine(), "Argument '" + argName + "' has already been declared.");
		}
		else
		{
			rec->addVariable(argName);
			func.numArguments++;
		}
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			addFunctionArguments(def, node->getChild(i), func);
	}
}
// --------------------------------------------------------------------------------
void ParseTree::addFunctions(EmitterDefinition* def, ParseTreeNode* node)
{
	if (node->getType() == PT_Function)
	{
		String funcName = node->getChild(0)->getStringData();
		
		if (def->getFunctionIndex(funcName) >= 0)
		{
			addError(node->getLine(), "Function '" + funcName + "' has already been declared.");
			return;
		}

		EmitterDefinition::Function& func = def->addFunction(funcName, node);
		
		// Create CodeRecord in ScriptMachine
		int index = createCodeRecord("Emitter", def->getName(), "Function", funcName);
		mFunctionIndices.push_back(index);

		// Get arguments
		if (node->getChild(1))
			addFunctionArguments(def, node->getChild(1), func);
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			addFunctions(def, node->getChild(i));
	}
}
// --------------------------------------------------------------------------------
void ParseTree::buildFunctions(EmitterDefinition* def, ParseTreeNode* node)
{
	static EmitterDefinition::Function* s_curFunc = 0;

	switch (node->getType())
	{
	case PT_Function:
		{
			String funcName = node->getChild(0)->getStringData();
			s_curFunc = &(def->getFunction(def->getFunctionIndex(funcName))); // dodgy but will be ok here
		}
		break;

	case PT_AssignStatement:
		{
			String varName = node->getChild(0)->getStringData();

			// See if it's a member or global
			if (def->getMemberVariableIndex(varName) >= 0)
			{
				addError(node->getLine(), "Cannot use member variables in control functions.");
				break;
			}
			else if (mScriptMachine->getGlobalVariableIndex(varName) >= 0)
			{
				addError(node->getLine(), "'" + varName + "' is read-only.");
				break;
			}

			// If it isn't, it's a local, so see if we need to create it.
			CodeRecord* rec = getCodeRecord("Emitter", def->getName(), "Function", s_curFunc->name);
			if (rec->getVariableIndex(varName) < 0)
				rec->addVariable(varName);
		}
		break;

	case PT_FireStatement:
		{
			// Make sure that the fire function is registered.
			String funcName = node->getChild(0)->getStringData();
			String funcType = node->getStringData();

			FireType* ft = mScriptMachine->getFireType(funcType);
			if (!ft)
			{
				addError(node->getLine(), "Unknown fire type ' " + funcType + "'.");
				return;
			}
			else if (!ft->fireFunctionExists(funcName))
			{
				addError(node->getLine(), "Fire function '" + funcName + "' is not registered.");
				return;
			}

			// Arguments
			int numArguments = 0;
			if (node->getChild(1))
				countFunctionCallArguments(node->getChild(1), numArguments);

			int expectedArgs = ft->getNumFireFunctionArguments(funcName);
			if (numArguments != expectedArgs)
			{
				std::stringstream ss;
				ss << "Function '" << funcName << "' expects " << expectedArgs << " arguments.";
				addError(node->getLine(), ss.str());
				return;
			}
			// FireType parameters
			if (node->getChild(3))
			{
				int numCtrls = 0;
				checkFireControllers(def, node->getChild(3), numCtrls, ft);
			}
		}
		break;

	case PT_Property:
		{
			String propName = node->getStringData();
			if (mScriptMachine->getPropertyIndex(propName) < 0)
				mScriptMachine->addProperty(propName);
		}
		break;

	case PT_Identifier:
		{
			int pType = node->getParent()->getType();
			String varName = node->getStringData();
			
			if (pType >= PT_ConstantExpression && pType <= PT_UnaryNegStatement)
			{
				// Local, members, globals
				CodeRecord* rec = getCodeRecord("Emitter", def->getName(), "Function", s_curFunc->name);
				if (rec->getVariableIndex(varName) < 0)
				{
					if (def->getMemberVariableIndex(varName) < 0)
					{
						if (mScriptMachine->getGlobalVariableIndex(varName) < 0)
						{
							addError(node->getLine(), "Variable '" + varName + "' is not declared.");
						}
					}
					else
					{
						addError(node->getLine(), "Cannot use member variables in functions.");
						break;
					}
				}
			}
		}
		break;

	case PT_LoopStatement:
		{
			// Check to make sure we don't loop too deeply.
			if (node->getChild(1))
			{
				int loopDepth = 0;
				checkLoopDepth(node, loopDepth);
			}
		}
		break;

	default:
		break;
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			buildFunctions(def, node->getChild(i));
	}
}
// --------------------------------------------------------------------------------
void ParseTree::createEmitterStates(EmitterDefinition* def, ParseTreeNode* node)
{
	switch (node->getType())
	{
	case PT_State:
		{
			EmitterDefinition::State st;

			String stateName = node->getChild(0)->getStringData();
			if (def->getStateIndex(stateName) >= 0)
			{
				addError(node->getLine(), "State '" + stateName + "' has already been declared.");
				return;
			}

			def->addState(stateName);

			// Create CodeRecord in ScriptMachine
			int index = createCodeRecord("Emitter", def->getName(), "State", stateName);
			mStateIndices.push_back(index);
		}
		break;

	case PT_AssignStatement:
		{
			String varName = node->getChild(0)->getStringData();

			// See if it's a member or global
			if (def->getMemberVariableIndex(varName) >= 0)
			{
				int mvIndex = def->getMemberVariableIndex(varName);
				const EmitterDefinition::MemberVariable& mv = def->getMemberVariable(mvIndex);
				if (mv.readonly)
				{
					addError(node->getLine(), "'" + varName + "' is read-only.");
				}
				break;
			}
			else if (mScriptMachine->getGlobalVariableIndex(varName) >= 0)
			{
				addError(node->getLine(), "'" + varName + "' is read-only.");
				break;
			}

			// If it isn't, it's a local, so see if we need to create it.
			int numStates = (int) def->getNumStates();
			EmitterDefinition::State& st = def->getState(numStates - 1);

			CodeRecord* rec = getCodeRecord("Emitter", def->getName(), "State", st.name);

			if (rec->getVariableIndex(varName) < 0)
				rec->addVariable(varName);
		}
		break;

	case PT_FireStatement:
		{
			// Make sure that the fire function is registered.
			String funcName = node->getChild(0)->getStringData();
			String funcType = node->getStringData();

			FireType* ft = mScriptMachine->getFireType(funcType);
			if (!ft)
			{
				addError(node->getLine(), "Unknown fire type ' " + funcType + "'.");
				return;
			}
			else if (!ft->fireFunctionExists(funcName))
			{
				addError(node->getLine(), "Fire function '" + funcName + "' is not registered.");
				return;
			}

			// Arguments
			int numArguments = 0;
			if (node->getChild(1))
				countFunctionCallArguments(node->getChild(1), numArguments);

			int expectedArgs = ft->getNumFireFunctionArguments(funcName);
			if (numArguments != expectedArgs)
			{
				std::stringstream ss;
				ss << "Function '" << funcName << "' expects " << expectedArgs << " arguments.";
				addError(node->getLine(), ss.str());
				return;
			}

			// FireType parameters
			if (node->getChild(3))
			{
				int numCtrls = 0;
				checkFireControllers(def, node->getChild(3), numCtrls, ft);
			}
		}
		break;

	case PT_Property:
		{
			addError(node->getLine(), "Properties can only be used in functions.");
		}
		break;

	case PT_Identifier:
		{
			int pType = node->getParent()->getType();
			String varName = node->getStringData();
			
			if (pType >= PT_ConstantExpression && pType <= PT_UnaryNegStatement)
			{
				// Local, members, globals
				int numStates = (int) def->getNumStates();
				EmitterDefinition::State& st = def->getState(numStates - 1);
				CodeRecord* rec = getCodeRecord("Emitter", def->getName(), "State", st.name);
				if (rec->getVariableIndex(varName) < 0)
				{
					if (def->getMemberVariableIndex(varName) < 0)
					{
						if (mScriptMachine->getGlobalVariableIndex(varName) < 0)
							addError(node->getLine(), "Variable '" + varName + "' is not declared.");
					}
				}
			}
		}
		break;

	case PT_LoopStatement:
		{
			// Check to make sure we don't loop too deeply.
			if (node->getChild(1))
			{
				int loopDepth = 0;
				checkLoopDepth(node, loopDepth);
			}
		}
		break;

	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			createEmitterStates(def, node->getChild(i));
	}
}
// --------------------------------------------------------------------------------
void ParseTree::createControllerStates(ControllerDefinition* def, ParseTreeNode* node)
{
	switch (node->getType())
	{
	case PT_State:
		{
			ControllerDefinition::State st;

			String stateName = node->getChild(0)->getStringData();
			if (def->getStateIndex(stateName) >= 0)
			{
				addError(node->getLine(), "State '" + stateName + "' has already been declared.");
				return;
			}

			def->addState(stateName);

			// Create CodeRecord in ScriptMachine
			int index = createCodeRecord("Controller", def->getName(), "State", stateName);
			mStateIndices.push_back(index);
		}
		break;

	case PT_AssignStatement:
		{
			String varName = node->getChild(0)->getStringData();

			if (node->getChild(2))
			{
				// We're setting an emitter member variable
				// ...
			}

			// See if it's a member or global
			if (def->getMemberVariableIndex(varName) >= 0)
			{
				int mvIndex = def->getMemberVariableIndex(varName);
				const ControllerDefinition::MemberVariable& mv = def->getMemberVariable(mvIndex);
				if (mv.readonly)
				{
					addError(node->getLine(), "'" + varName + "' is read-only.");
				}
				break;
			}
			else if (mScriptMachine->getGlobalVariableIndex(varName) >= 0)
			{
				addError(node->getLine(), "'" + varName + "' is read-only.");
				break;
			}

			// If it isn't, it's a local, so see if we need to create it.
			int numStates = (int) def->getNumStates();
			ControllerDefinition::State& st = def->getState(numStates - 1);

			CodeRecord* rec = getCodeRecord("Controller", def->getName(), "State", st.name);

			if (rec->getVariableIndex(varName) < 0)
				rec->addVariable(varName);
		}
		break;

	case PT_Property:
		{
			addError(node->getLine(), "Properties can only be used in functions.");
		}
		break;

	case PT_Identifier:
		{
			int pType = node->getParent()->getType();
			String varName = node->getStringData();
			
			if (pType >= PT_ConstantExpression && pType <= PT_UnaryNegStatement)
			{
				// Local, members, globals
				int numStates = (int) def->getNumStates();
				ControllerDefinition::State& st = def->getState(numStates - 1);
				CodeRecord* rec = getCodeRecord("Controller", def->getName(), "State", st.name);
				if (rec->getVariableIndex(varName) < 0)
				{
					if (def->getMemberVariableIndex(varName) < 0)
					{
						if (mScriptMachine->getGlobalVariableIndex(varName) < 0)
							addError(node->getLine(), "Variable '" + varName + "' is not declared.");
					}
				}
			}
		}
		break;

	case PT_LoopStatement:
		{
			// Check to make sure we don't loop too deeply.
			if (node->getChild(1))
			{
				int loopDepth = 0;
				checkLoopDepth(node, loopDepth);
			}
		}
		break;

	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			createControllerStates(def, node->getChild(i));
	}
}
// --------------------------------------------------------------------------------
void ParseTree::_checkFireStatements(EmitterDefinition* def, ParseTreeNode* node, const String& type)
{
	int nodeType = node->getType();
	if (nodeType == PT_FunctionCall)
	{
		FireType* ft = mScriptMachine->getFireType(type);

		// Make sure that any control function used has the correct properties
		String funcName = node->getChild(0)->getStringData();
		EmitterDefinition::Function func = def->getFunction(def->getFunctionIndex(funcName));
		checkFunctionProperties(func.node, ft);
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			_checkFireStatements(def, node->getChild(i), type);
	}
}
// -------------------------------------------------------------------------
void ParseTree::checkFireStatements(EmitterDefinition* def, ParseTreeNode* node)
{
	if (node->getType() == PT_FireStatement)
	{
		ParseTreeNode* tailNode = node->getChild(3);
		if (tailNode)
		{
			// Loop through list
			String typeName = node->getStringData();
			_checkFireStatements(def, tailNode, typeName);
		}
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			checkFireStatements(def, node->getChild(i));
	}
}
// --------------------------------------------------------------------------------
void ParseTree::checkFunctionProperties(ParseTreeNode* node, FireType* type)
{
	if (node->getType() == PT_Property)
	{
		String prop = node->getStringData();
		if (type->getPropertyIndex(prop) < 0)
			addError(node->getLine(), "Property '" + prop + "' not registered for " + type->getName());
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			checkFunctionProperties(node->getChild(i), type);
	}
}
// --------------------------------------------------------------------------------
void ParseTree::setAffectorRecalculationType(EmitterDefinition* def, Affector* affector, 
											 ParseTreeNode* node)
{
	switch (node->getType())
	{
	case PT_FunctionCall:
		// Always recalculate every single time the affector is applied.  Only use this
		// option if you absolutely must!
		affector->recalculateAlways(true);
		return;

	case PT_Identifier:
		// Variable, either global or member
		{
			String varName = node->getStringData();
			if (def->getMemberVariableIndex(varName) >= 0)
			{
				int mvIndex = def->getMemberVariableIndex(varName);
				const EmitterDefinition::MemberVariable& mv = def->getMemberVariable(mvIndex);

				// Different Emitter instances will be using a particular Affector instance, and when
				// a FireType runs an affector, it will need to know which Emitter it was fired from,
				// and if that Emitter has been signalled as modified (by member var change) then recalc.

				
			}
			else if (mScriptMachine->getGlobalVariableIndex(varName) >= 0)
			{
				// Recalculate when global variable changes value
				GlobalVariable* gv = mScriptMachine->getGlobalVariable(varName);
				gv->registerListener(affector);
			}
		}
		break;
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			setAffectorRecalculationType(def, affector, node->getChild(i));
	}
}
// --------------------------------------------------------------------------------
void ParseTree::createMemberVariableBytecode(EmitterDefinition* def, ParseTreeNode* node, bool first)
{
	static int s_index = 0;
	if (first)
		s_index = NUM_SPECIAL_MEMBERS + def->getNumUserMembers();

	if (node->getType() == PT_AssignStatement)
	{
		// Create 'constructor' code for this emitter.  We only need to do this if member 
		// variables are not constants.  If they are, then we can just set the constants here.
		EmitterDefinition::MemberVariable& memVar = def->getMemberVariable(s_index);
		
		int exprType = node->getChild(1)->getType();
		if (exprType == PT_ConstantExpression)
		{
			memVar.value = 0;

			// Add to 'construction code'
			BytecodeBlock constructCode;
			generateBytecode(def, node->getChild(1), &constructCode);
			
			int index = def->getMemberVariableIndex(memVar.name);
			constructCode.push_back(BC_SETM);
			constructCode.push_back((uint32) index);

			def->appendConstructionCode(constructCode);
		}
		else
		{
			// Constant - just set the variable now
			memVar.value = node->getChild(1)->getValueData(); 
		}

		++s_index;
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			createMemberVariableBytecode(def, node->getChild(i), false);
	}
}
// --------------------------------------------------------------------------------
void ParseTree::generateFireTail(EmitterDefinition* def, ParseTreeNode* node, 
								 BytecodeBlock* bytecode, FireType* ft)
{
	int nodeType = node->getType();
	if (nodeType == PT_FunctionCall)
	{
		ParseTreeNode* argNode = node->getChild(1);
		if (argNode)
			generateBytecode(def, argNode, bytecode);
	}
	else if (nodeType == PT_AffectorCall)
	{

		// See if affector instance has been created in FireType, and if it has, use 
		// its index, otherwise add it and use its index.
		String affector = node->getStringData();
		int affIndex = -1;
		for (size_t i = 0; i < mAffectors.size(); ++i)
		{
			if (mAffectors[i].name == affector)
			{
				affIndex = (int) i;
				break;
			}
		}

		// See FireType::getControllers
		String instanceName = def->getName() + "-" + affector;

		int instanceIndex = ft->getAffectorInstanceIndex(instanceName);
		if (instanceIndex < 0)
		{
			// Generate CodeRecord and give to FireType
			BytecodeBlock argCode;
			ParseTreeNode* argsNode = mAffectors[affIndex].node->getChild(1)->getChild(1);
			generateBytecode(def, argsNode, &argCode);

			String affFunction = mAffectors[affIndex].function;
			instanceIndex = ft->addAffectorInstance(instanceName, 
													ft->getAffectorFunction(affFunction), 
													mAffectors[affIndex].numArgs,
													argCode,
													mScriptMachine);

			// Check if the affector arguments use functions, global or member variables.
			Affector* affector = ft->getAffectorInstance(instanceIndex);
			setAffectorRecalculationType(def, affector, argsNode);
		}
	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			generateFireTail(def, node->getChild(i), bytecode, ft);
	}
}
// --------------------------------------------------------------------------------
void ParseTree::generateBytecode(EmitterDefinition* def, ParseTreeNode* node,
								 BytecodeBlock* bytecode, bool reset)
{
	static EmitterDefinition::State* s_curState = 0;
	static EmitterDefinition::Function* s_curFunction = 0;
	static bool s_stateNotFunction = true;

	if (reset)
	{
		s_curState = 0;
		s_curFunction = 0;
		s_stateNotFunction = true;
	}

	// Top-down nodes
	switch (node->getType())
	{
	case PT_State:
		{
			String stateName = node->getChild(0)->getStringData();
			s_curState = &(def->getState(def->getStateIndex(stateName))); // dodgy, but ok
			s_stateNotFunction = true;

			// Generate code
			if (node->getChild(1))
			{
				BytecodeBlock stateByteCode;
				generateBytecode(def, node->getChild(1), &stateByteCode);
				
				// Give to ScriptMachine
				CodeRecord* rec = getCodeRecord("Emitter", def->getName(), "State", stateName);
				rec->byteCodeSize = stateByteCode.size();
				rec->byteCode = new uint32[rec->byteCodeSize];
				for (uint32 i = 0; i < rec->byteCodeSize; ++i)
					rec->byteCode[i] = stateByteCode[i];
			}
		}
		return;

	case PT_Function:
		{
			String funcName = node->getChild(0)->getStringData();
			s_curFunction = &(def->getFunction(def->getFunctionIndex(funcName))); // dodgy, but ok
			s_stateNotFunction = false;

			// Generate code
			if (node->getChild(2))
			{
				BytecodeBlock funcByteCode;
				generateBytecode(def, node->getChild(2), &funcByteCode);
				
				// Give to ScriptMachine
				CodeRecord* rec = getCodeRecord("Emitter", def->getName(), "Function", funcName);
				rec->byteCodeSize = funcByteCode.size();
				rec->byteCode = new uint32[rec->byteCodeSize];
				for (uint32 i = 0; i < rec->byteCodeSize; ++i)
					rec->byteCode[i] = funcByteCode[i];
			}
		}
		return;

	case PT_MemberList:
		{
			createMemberVariableBytecode(def, node, true);
		}
		return;

	case PT_AssignStatement:
		{
			String varName = node->getChild(0)->getStringData ();
			// Make sure we're not assigning to a global
			if (mScriptMachine->getGlobalVariableIndex(varName) >= 0)
			{
				// Error, globals are read-only - however it should not get this far.
				addError(node->getLine(), varName + " is read-only.");
				break;
			}

			// Generate value
			generateBytecode(def, node->getChild(1), bytecode);

			// Set variable - see if it is a member first
			if (def->getMemberVariableIndex(varName) >= 0)
			{
				int index = def->getMemberVariableIndex(varName);
				bytecode->push_back(BC_SETM);
				bytecode->push_back((uint32) index);
			}
			else
			{
				// Local variable
				CodeRecord* rec;
				int index;
				if (s_stateNotFunction)
					rec = getCodeRecord("Emitter", def->getName(), "State", s_curState->name);
				else
					rec = getCodeRecord("Emitter", def->getName(), "Function", s_curFunction->name);

				index = rec->getVariableIndex(varName);

				bytecode->push_back(BC_SETL);
				bytecode->push_back((uint32) index);
			}
		}
		return;

	case PT_FireStatement:
		{
			String funcType = node->getStringData();
			FireType* ft = mScriptMachine->getFireType(funcType);

			// Generate FireType function arguments and create affector instances,
			// to be used by FireType::generateBytecode
			ParseTreeNode* tNode = node->getChild(3);
			if (tNode)
				generateFireTail(def, tNode, bytecode, ft);

			// Generate fire function arguments next
			if (node->getChild(1))
				generateBytecode(def, node->getChild(1), bytecode);

			// Then generate actual BC_FIRE code
			String funcName = node->getChild(0)->getStringData();
			ft->generateBytecode(def, node, bytecode, funcName);
		}
		return;

	case PT_DieStatement:
		{
			// We don't know whether this is a bullet or area function
			bytecode->push_back(BC_DIE);
		}
		break;

	case PT_IfStatement:
		{
			// Generate test expression
			generateBytecode(def, node->getChild(0), bytecode);

			bytecode->push_back(BC_JZ);
			size_t jumpPos = bytecode->size();
			bytecode->push_back(0); // dummy jump address

			// Generate 'if' code
			if (node->getChild(1))
				generateBytecode(def, node->getChild(1), bytecode);

			// Generate 'else' code
			if (node->getChild(2))
			{
				// Jump to end if we're coming from 'if'
				bytecode->push_back(BC_JUMP);
				size_t endJumpPos = bytecode->size();
				bytecode->push_back(0); // dummy jump address

				(*bytecode)[jumpPos] = (uint32) bytecode->size();
				generateBytecode(def, node->getChild(2), bytecode);
				(*bytecode)[endJumpPos] = (uint32) bytecode->size();
			}
			else
			{
				(*bytecode)[jumpPos] = (uint32) bytecode->size();
			}
		}
		return;

	case PT_LoopStatement:
		{
			// Loop takes the counter off the stack, and then loops the code from
			// its (position+2) to the address specified in (position+1)
			// To loop indefinitely, just use a state, and goto to break.

			// Generate counter value
			generateBytecode(def, node->getChild(0), bytecode);

			bytecode->push_back(BC_LOOP);
			size_t endJumpPos = bytecode->size();
			bytecode->push_back(0); // dummy jump address

			// Generate code
			if (node->getChild(1))
				generateBytecode(def, node->getChild(1), bytecode);

			(*bytecode)[endJumpPos] = (uint32) bytecode->size();			
		}
		return;

	case PT_WhileStatement:
		{
			// Generate test expression
			size_t startJumpPos = bytecode->size();
			generateBytecode(def, node->getChild(0), bytecode);

			bytecode->push_back(BC_JZ);
			size_t endJumpPos = bytecode->size();
			bytecode->push_back(0); // dummy jump address

			// Generate code
			if (node->getChild(1))
				generateBytecode(def, node->getChild(1), bytecode);

			bytecode->push_back(BC_JUMP);
			bytecode->push_back(startJumpPos);
			(*bytecode)[endJumpPos] = (uint32) bytecode->size();	
		}
		return;

	case PT_GotoStatement:
		{
			String stateName = node->getChild(0)->getStringData();

			int index = getCodeRecordIndex("Emitter", def->getName(), "State", stateName);
			if (index >= 0)
			{
				bytecode->push_back(BC_GOTO);
				bytecode->push_back((uint32) index);
			}
			else
			{
				addError(node->getLine(), "State '" + stateName + "' has not been declared.");
			}
		}
		return;

	case PT_WaitStatement:
		{
			if (!node->getChild(0))
			{
				// Error, must have arguments
				addError(node->getLine(), "Wait statement has no arguments.");
				return;
			}
			
			generateBytecode(def, node->getChild(0), bytecode);
			bytecode->push_back(BC_WAIT);
		}
		return;

	case PT_SetStatement:
		{
			// Generate constant expression for value
			generateBytecode(def, node->getChild(1), bytecode);

			if (node->getChild(2))
			{
				// Generate constant expression for time
				generateBytecode(def, node->getChild(2), bytecode);
				bytecode->push_back(BC_SETPROPERTY2);
			}
			else
			{
				bytecode->push_back(BC_SETPROPERTY1);
			}

			String propName = node->getChild(0)->getStringData();
			int propIndex = mScriptMachine->getPropertyIndex(propName);
			bytecode->push_back(propIndex);
		}
		return;

	case PT_FunctionCall:
		{
			// Generate arguments first
			if (node->getChild(1))
				generateBytecode(def, node->getChild(1), bytecode);

			String funcName = node->getChild(0)->getStringData();

			// See if it's a native function
			int index = mScriptMachine->getNativeFunctionIndex(funcName);

			if (index < 0)
			{
				addError(node->getLine(), "Function '" + funcName + "' not found.");

				return;
			}

			bytecode->push_back(BC_CALL);
			bytecode->push_back((uint32) index);
		}
		return;

	case PT_Constant:
		{
			// Push value
			bstype val = node->getValueData();
			bytecode->push_back(BC_PUSH);
			bytecode->push_back(BS_TYPE_TO_UINT32(val));
		}
		break;

	case PT_Property:
		{
			// Get property
			bytecode->push_back(BC_GETPROPERTY);
			String propName = node->getStringData();
			int propIndex = mScriptMachine->getPropertyIndex(propName);
			bytecode->push_back(propIndex);
		}
		break;

	case PT_Identifier:
		{
			String varName = node->getStringData();

			// Check for local, then member, then global
			int index;
			CodeRecord* rec;
			if (s_stateNotFunction)
				rec = getCodeRecord("Emitter", def->getName(), "State", s_curState->name);
			else
				rec = getCodeRecord("Emitter", def->getName(), "Function", s_curFunction->name);
			index = rec->getVariableIndex(varName);

			if (index >= 0)
			{
				bytecode->push_back(BC_GETL);
				bytecode->push_back((uint32) index);
			}
			else if (def->getMemberVariableIndex(varName) >= 0)
			{
				int index = def->getMemberVariableIndex(varName);
				bytecode->push_back(BC_GETM);
				bytecode->push_back((uint32) index);
			}
			else
			{
				int globalIndex = mScriptMachine->getGlobalVariableIndex(varName);
				if (globalIndex >= 0)
				{
					bytecode->push_back(BC_GETG);
					bytecode->push_back((uint32) globalIndex);
				}
				else
				{
					addError(node->getLine(), "Variable '" + varName + "' is not declared.");
				}
			}
		}
		break;

	default:
		break;

	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			generateBytecode(def, node->getChild(i), bytecode, reset);
	}

	// Bottom-up nodes
	switch (node->getType())
	{
	case PT_UnaryPosStatement:
		bytecode->push_back(BC_OP_POS);
		break;

	case PT_UnaryNegStatement:
		bytecode->push_back(BC_OP_NEG);
		break;

	case PT_AddStatement:
		bytecode->push_back(BC_OP_ADD);
		break;

	case PT_SubtractStatement:
		bytecode->push_back(BC_OP_SUBTRACT);
		break;

	case PT_MultiplyStatement:
		bytecode->push_back(BC_OP_MULTIPLY);
		break;

	case PT_DivideStatement:
		bytecode->push_back(BC_OP_DIVIDE);
		break;

	case PT_RemainderStatement:
		bytecode->push_back(BC_OP_REMAINDER);
		break;

	case PT_LessThanStatement:
		bytecode->push_back(BC_OP_LT);
		break;

	case PT_LessThanEqStatement:
		bytecode->push_back(BC_OP_LTE);
		break;

	case PT_GreaterThanStatement:
		bytecode->push_back(BC_OP_GT);
		break;

	case PT_GreaterThanEqStatement:
		bytecode->push_back(BC_OP_GTE);
		break;

	case PT_EqualsStatement:
		bytecode->push_back(BC_OP_EQ);
		break;

	case PT_NotEqualsStatement:
		bytecode->push_back(BC_OP_NEQ);
		break;

	case PT_LogicalOr:
		bytecode->push_back(BC_LOG_OR);
		break;

	case PT_LogicalAnd:
		bytecode->push_back(BC_LOG_AND);
		break;
	}
}
// --------------------------------------------------------------------------------
void ParseTree::generateBytecode(ControllerDefinition* def, ParseTreeNode* node,
								 BytecodeBlock* bytecode, bool reset)
{
	static ControllerDefinition::State* s_curState = 0;
//	static ControllerDefinition::Event* s_curEvent = 0;
	static bool s_stateNotEvent = true;

	if (reset)
	{
		s_curState = 0;
//		s_curFunction = 0;
		s_stateNotEvent = true;
	}

	// Top-down nodes
	switch (node->getType())
	{
	case PT_State:
		{
			String stateName = node->getChild(0)->getStringData();
			s_curState = &(def->getState(def->getStateIndex(stateName))); // dodgy, but ok
			s_stateNotEvent = true;

			// Generate code
			if (node->getChild(1))
			{
				BytecodeBlock stateByteCode;
				generateBytecode(def, node->getChild(1), &stateByteCode);
				
				// Give to ScriptMachine
				CodeRecord* rec = getCodeRecord("Controller", def->getName(), "State", stateName);
				rec->byteCodeSize = stateByteCode.size();
				rec->byteCode = new uint32[rec->byteCodeSize];
				for (uint32 i = 0; i < rec->byteCodeSize; ++i)
					rec->byteCode[i] = stateByteCode[i];
			}
		}
		return;

	case PT_Event:
		{
		}
		return;

	case PT_MemberList:
		{
//			createMemberVariableBytecode(def, node, true);
		}
		return;

	case PT_AssignStatement:
		{
			String varName = node->getChild(0)->getStringData ();
			// Make sure we're not assigning to a global
			if (mScriptMachine->getGlobalVariableIndex(varName) >= 0)
			{
				// Error, globals are read-only - however it should not get this far.
				addError(node->getLine(), varName + " is read-only.");
				break;
			}

			// Generate value
			generateBytecode(def, node->getChild(1), bytecode);

			// Set variable - see if it is a member first
			if (def->getMemberVariableIndex(varName) >= 0)
			{
				int index = def->getMemberVariableIndex(varName);
				bytecode->push_back(BC_SETM);
				bytecode->push_back((uint32) index);
			}
			else
			{
				// Local variable
				CodeRecord* rec;
				int index;
				if (s_stateNotEvent)
					rec = getCodeRecord("Controller", def->getName(), "State", s_curState->name);
//				else
//					rec = getCodeRecord("Controller", def->getName(), "Event", s_curEvent->name);

				index = rec->getVariableIndex(varName);

				bytecode->push_back(BC_SETL);
				bytecode->push_back((uint32) index);
			}
		}
		return;

	case PT_IfStatement:
		{
			// Generate test expression
			generateBytecode(def, node->getChild(0), bytecode);

			bytecode->push_back(BC_JZ);
			size_t jumpPos = bytecode->size();
			bytecode->push_back(0); // dummy jump address

			// Generate 'if' code
			if (node->getChild(1))
				generateBytecode(def, node->getChild(1), bytecode);

			// Generate 'else' code
			if (node->getChild(2))
			{
				// Jump to end if we're coming from 'if'
				bytecode->push_back(BC_JUMP);
				size_t endJumpPos = bytecode->size();
				bytecode->push_back(0); // dummy jump address

				(*bytecode)[jumpPos] = (uint32) bytecode->size();
				generateBytecode(def, node->getChild(2), bytecode);
				(*bytecode)[endJumpPos] = (uint32) bytecode->size();
			}
			else
			{
				(*bytecode)[jumpPos] = (uint32) bytecode->size();
			}
		}
		return;

	case PT_LoopStatement:
		{
			// Loop takes the counter off the stack, and then loops the code from
			// its (position+2) to the address specified in (position+1)
			// To loop indefinitely, just use a state, and goto to break.

			// Generate counter value
			generateBytecode(def, node->getChild(0), bytecode);

			bytecode->push_back(BC_LOOP);
			size_t endJumpPos = bytecode->size();
			bytecode->push_back(0); // dummy jump address

			// Generate code
			if (node->getChild(1))
				generateBytecode(def, node->getChild(1), bytecode);

			(*bytecode)[endJumpPos] = (uint32) bytecode->size();			
		}
		return;

	case PT_WhileStatement:
		{
			// Generate test expression
			size_t startJumpPos = bytecode->size();
			generateBytecode(def, node->getChild(0), bytecode);

			bytecode->push_back(BC_JZ);
			size_t endJumpPos = bytecode->size();
			bytecode->push_back(0); // dummy jump address

			// Generate code
			if (node->getChild(1))
				generateBytecode(def, node->getChild(1), bytecode);

			bytecode->push_back(BC_JUMP);
			bytecode->push_back(startJumpPos);
			(*bytecode)[endJumpPos] = (uint32) bytecode->size();	
		}
		return;

	case PT_GotoStatement:
		{
			String stateName = node->getChild(0)->getStringData();

			int index = getCodeRecordIndex("Controller", def->getName(), "State", stateName);
			if (index >= 0)
			{
				bytecode->push_back(BC_GOTO);
				bytecode->push_back((uint32) index);
			}
			else
			{
				addError(node->getLine(), "State '" + stateName + "' has not been declared.");
			}
		}
		return;

	case PT_WaitStatement:
		{
			if (!node->getChild(0))
			{
				// Error, must have arguments
				addError(node->getLine(), "Wait statement has no arguments.");
				return;
			}
			
			generateBytecode(def, node->getChild(0), bytecode);
			bytecode->push_back(BC_WAIT);
		}
		return;

	case PT_FunctionCall:
		{
			// Generate arguments first
			if (node->getChild(1))
				generateBytecode(def, node->getChild(1), bytecode);

			String funcName = node->getChild(0)->getStringData();

			// See if it's a native function
			int index = mScriptMachine->getNativeFunctionIndex(funcName);

			if (index < 0)
			{
				addError(node->getLine(), "Function '" + funcName + "' not found.");

				return;
			}

			bytecode->push_back(BC_CALL);
			bytecode->push_back((uint32) index);
		}
		return;

	case PT_Constant:
		{
			// Push value
			bstype val = node->getValueData();
			bytecode->push_back(BC_PUSH);
			bytecode->push_back(BS_TYPE_TO_UINT32(val));
		}
		break;

	case PT_Identifier:
		{
			String varName = node->getStringData();

			// Check for local, then member, then global
			int index;
			CodeRecord* rec;
			if (s_stateNotEvent)
				rec = getCodeRecord("Controller", def->getName(), "State", s_curState->name);
//			else
//				rec = getCodeRecord("Controller", def->getName(), "Event", s_curEvent->name);
			index = rec->getVariableIndex(varName);

			if (index >= 0)
			{
				bytecode->push_back(BC_GETL);
				bytecode->push_back((uint32) index);
			}
			else if (def->getMemberVariableIndex(varName) >= 0)
			{
				int index = def->getMemberVariableIndex(varName);
				bytecode->push_back(BC_GETM);
				bytecode->push_back((uint32) index);
			}
			else
			{
				int globalIndex = mScriptMachine->getGlobalVariableIndex(varName);
				if (globalIndex >= 0)
				{
					bytecode->push_back(BC_GETG);
					bytecode->push_back((uint32) globalIndex);
				}
				else
				{
					addError(node->getLine(), "Variable '" + varName + "' is not declared.");
				}
			}
		}
		break;

	default:
		break;

	}

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		if (node->getChild(i))
			generateBytecode(def, node->getChild(i), bytecode, reset);
	}

	// Bottom-up nodes
	switch (node->getType())
	{
	case PT_UnaryPosStatement:
		bytecode->push_back(BC_OP_POS);
		break;

	case PT_UnaryNegStatement:
		bytecode->push_back(BC_OP_NEG);
		break;

	case PT_AddStatement:
		bytecode->push_back(BC_OP_ADD);
		break;

	case PT_SubtractStatement:
		bytecode->push_back(BC_OP_SUBTRACT);
		break;

	case PT_MultiplyStatement:
		bytecode->push_back(BC_OP_MULTIPLY);
		break;

	case PT_DivideStatement:
		bytecode->push_back(BC_OP_DIVIDE);
		break;

	case PT_RemainderStatement:
		bytecode->push_back(BC_OP_REMAINDER);
		break;

	case PT_LessThanStatement:
		bytecode->push_back(BC_OP_LT);
		break;

	case PT_LessThanEqStatement:
		bytecode->push_back(BC_OP_LTE);
		break;

	case PT_GreaterThanStatement:
		bytecode->push_back(BC_OP_GT);
		break;

	case PT_GreaterThanEqStatement:
		bytecode->push_back(BC_OP_GTE);
		break;

	case PT_EqualsStatement:
		bytecode->push_back(BC_OP_EQ);
		break;

	case PT_NotEqualsStatement:
		bytecode->push_back(BC_OP_NEQ);
		break;

	case PT_LogicalOr:
		bytecode->push_back(BC_LOG_OR);
		break;

	case PT_LogicalAnd:
		bytecode->push_back(BC_LOG_AND);
		break;
	}
}
// --------------------------------------------------------------------------------
EmitterDefinition* ParseTree::createEmitterDefinition(ParseTreeNode* node,
													  const MemberVariableDeclarationMap& memberDecls)
{
	String name = node->getStringData();

	// Create definition
	EmitterDefinition* def = new EmitterDefinition(name);

	// Create pre-declared member variables first
	addEmitterMemberVariables(def, memberDecls);

	// Create script-declared member variables
	bool hasMembers = node->getChild(PT_EmitterMemberNode) ? true : false;
	if (hasMembers)
		createEmitterMemberVariables(def, node->getChild(PT_EmitterMemberNode));

	if (mNumErrors > 0)
	{
		delete def;
		return 0;
	}

	// Now check affectors
	mAffectors.clear();
	bool hasAffectors = node->getChild(PT_EmitterAffectorNode) ? true : false;
	if (hasAffectors)
	{
		createAffectors(def, node->getChild(PT_EmitterAffectorNode));

		if (mNumErrors > 0)
		{
			delete def;
			return 0;
		}
	}

	// Create functions
	mFunctionIndices.clear();
	bool hasFunctions = node->getChild(PT_EmitterFunctionNode) ? true : false;
	if (hasFunctions)
	{
		addFunctions(def, node->getChild(PT_EmitterFunctionNode));

		if (mNumErrors > 0)
		{
			delete def;
			return 0;
		}

		buildFunctions(def, node->getChild(PT_EmitterFunctionNode));

		if (mNumErrors > 0)
		{
			delete def;
			return 0;
		}
	}

	// Create the states
	mStateIndices.clear();
	createEmitterStates(def, node->getChild(PT_EmitterStateNode));
	
	if (mNumErrors > 0)
	{
		delete def;
		return 0;
	}

	// This is a bit of a hack, but now we have to check fire statements to make sure that
	// the functions they use do not contain invalid properties, and that they do not use a
	// function that has the same name as an affector. This is because we do not
	// know what type a function has been given from the function itself.
	checkFireStatements(def, node);

	if (mNumErrors > 0)
	{
		delete def;
		return 0;
	}

	// Set the maximum number of locals for the EmitterDefinition.  Take functions into
	// consideration here as well because they use the EmitterDefinition's info for creation.
	int maxLocals = -1;
	std::list<int>::iterator sit = mStateIndices.begin();
	while (sit != mStateIndices.end())
	{
		CodeRecord* rec = mScriptMachine->getCodeRecord(*sit);
		int numVars = rec->getNumVariables();
		if (maxLocals < numVars)
			maxLocals = numVars;
		++sit;
	}

	std::list<int>::iterator fit = mFunctionIndices.begin();
	while (fit != mFunctionIndices.end())
	{
		CodeRecord* rec = mScriptMachine->getCodeRecord(*fit);
		int numVars = rec->getNumVariables();
		if (maxLocals < numVars)
			maxLocals = numVars;
		++fit;
	}
	
	def->setInitialState(*(mStateIndices.begin()));
	def->setMaxLocalVariables(maxLocals);

	// Initialise member vars
	if (hasMembers)
		generateBytecode(def, node->getChild(PT_EmitterMemberNode), 0, true);

	// Finish constructor here
	def->finaliseConstructor();

	// Create function bytecode
	if (hasFunctions)
		generateBytecode(def, node->getChild(PT_EmitterFunctionNode), 0, true);

	// Create state bytecode
	generateBytecode(def, node->getChild(PT_EmitterStateNode), 0, true);

	if (mNumErrors > 0)
	{
		delete def;
		return 0;
	}
	else
	{
		//def->print(std::cerr);
/*
		for (int i = 0; i < mScriptMachine->getNumCodeRecords(); ++i)
		{
			CodeRecord* rec = mScriptMachine->getCodeRecord(i);
			for (size_t j = 0; j < rec->byteCodeSize; ++j)
				std::cout << rec->byteCode[j] << std::endl;
			std::cout << std::endl;
		}
*/
		return def;
	}
}
// --------------------------------------------------------------------------------
ControllerDefinition* ParseTree::createControllerDefinition(ParseTreeNode* node)
{
	String name = node->getStringData();

	// Create definition
	ControllerDefinition* def = new ControllerDefinition(name);

	// Create pre-declared member variables first
	addControllerMemberVariables(def);

	// Create script-declared member variables
	bool hasMembers = node->getChild(PT_EmitterMemberNode) ? true : false;
//	if (hasMembers)
//		createEmitterMemberVariables(def, node->getChild(PT_EmitterMemberNode));

	if (mNumErrors > 0)
	{
		delete def;
		return 0;
	}

	// Now check emitter variables
	bool hasEmitters = node->getChild(PT_ControllerEmitterNode) ? true : false;
	if (hasEmitters)
	{
		createEmitterVariables(def, node->getChild(PT_ControllerEmitterNode));

		if (mNumErrors > 0)
		{
			delete def;
			return 0;
		}
	}
/*
	// Create events
	mFunctionIndices.clear();
	bool hasFunctions = node->getChild(PT_EmitterFunctionNode) ? true : false;
	if (hasFunctions)
	{
		addFunctions(def, node->getChild(PT_EmitterFunctionNode));

		if (mNumErrors > 0)
		{
			delete def;
			return 0;
		}

		buildFunctions(def, node->getChild(PT_EmitterFunctionNode));

		if (mNumErrors > 0)
		{
			delete def;
			return 0;
		}
	}
*/
	// Create the states
	mStateIndices.clear();
	createControllerStates(def, node->getChild(PT_ControllerStateNode));
	
	if (mNumErrors > 0)
	{
		delete def;
		return 0;
	}

	// Set the maximum number of locals for the ControllerDefinition
	int maxLocals = -1;
	std::list<int>::iterator sit = mStateIndices.begin();
	while (sit != mStateIndices.end())
	{
		CodeRecord* rec = mScriptMachine->getCodeRecord(*sit);
		int numVars = rec->getNumVariables();
		if (maxLocals < numVars)
			maxLocals = numVars;
		++sit;
	}
	
	def->setInitialState(*(mStateIndices.begin()));
	def->setMaxLocalVariables(maxLocals);

	// Initialise member vars
	if (hasMembers)
		generateBytecode(def, node->getChild(PT_ControllerMemberNode), 0, true);

	// Finish constructor here
	def->finaliseConstructor();

	// Create state bytecode
	generateBytecode(def, node->getChild(PT_ControllerStateNode), 0, true);

	if (mNumErrors > 0)
	{
		delete def;
		return 0;
	}
	else
	{
		//def->print(std::cerr);
/*
		for (int i = 0; i < mScriptMachine->getNumCodeRecords(); ++i)
		{
			CodeRecord* rec = mScriptMachine->getCodeRecord(i);
			for (size_t j = 0; j < rec->byteCodeSize; ++j)
				std::cout << rec->byteCode[j] << std::endl;
			std::cout << std::endl;
		}
*/
		return def;
	}
}
// --------------------------------------------------------------------------------
void ParseTree::createEmitterDefinitions(ParseTreeNode* node, 
										 const MemberVariableDeclarationMap& memberDecls)
{
	if (node->getType() == PT_EmitterDefinition)
	{
		EmitterDefinition* def = createEmitterDefinition(node, memberDecls);
		if (def)
			mScriptMachine->addEmitterDefinition(def->getName(), def);
	}
	else
	{
		for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++i)
		{
			if (node->getChild(i))
				createEmitterDefinitions(node->getChild(i), memberDecls);
		}
	}
}
// --------------------------------------------------------------------------------
void ParseTree::createControllerDefinitions(ParseTreeNode* node)
{
	if (node->getType() == PT_ControllerDefinition)
	{
		ControllerDefinition* def = createControllerDefinition(node);
		if (def)
			mScriptMachine->addControllerDefinition(def->getName(), def);
	}
	else
	{
		for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++i)
		{
			if (node->getChild(i))
				createControllerDefinitions(node->getChild(i));
		}
	}
}
// --------------------------------------------------------------------------------
void ParseTree::createDefinitions(ParseTreeNode* node,
								  const MemberVariableDeclarationMap& memberDecls)
{
	// Create emitter definitions first, then controller definitions, because 
	// controllers rely on emitters
	createEmitterDefinitions(node, memberDecls);
	createControllerDefinitions(node);
}
// --------------------------------------------------------------------------------
String ParseTree::getCodeRecordName(const String& type, const String& typeName,
									const String& blockType, const String& blockName) const
{
	String name = type + "-" + typeName + "-" + blockType + "-" + blockName;
	return name;
}
// --------------------------------------------------------------------------------
int ParseTree::createCodeRecord(const String& type, const String& typeName,
								const String& blockType, const String& blockName)
{
	String name = getCodeRecordName(type, typeName, blockType, blockName);

	int index = (int) mCodeblockNames.size();
	mCodeblockNames.push_back(name);
	mScriptMachine->createCodeRecord();

	return index;
}
// --------------------------------------------------------------------------------
CodeRecord* ParseTree::getCodeRecord(const String& type, const String& typeName,
									 const String& blockType, const String& blockName) const
{
	String name = getCodeRecordName(type, typeName, blockType, blockName);
	for (size_t i = 0; i < mCodeblockNames.size(); ++i)
		if (name == mCodeblockNames[i])
			return mScriptMachine->getCodeRecord((int) i);
	
	return 0;
}
// --------------------------------------------------------------------------------
int ParseTree::getCodeRecordIndex(const String& type, const String& typeName,
								 const String& blockType, const String& blockName) const
{
	String name = getCodeRecordName(type, typeName, blockType, blockName);
	for (size_t i = 0; i < mCodeblockNames.size(); ++i)
		if (name == mCodeblockNames[i])
			return (int) i;
	
	return -1;
}
// --------------------------------------------------------------------------------
void ParseTree::print(ParseTreeNode* node, int indent)
{
	for (int i = 0; i < indent; ++i)
		std::cout << " ";

	switch(node->getType())
	{
	case PT_EmitterDefinition:
		std::cout << "Emitter Definition: " << node->getChild(0)->getStringData();
		break;

	case PT_FunctionCall:
		std::cout << "Function Call: " << node->getChild(0)->getStringData();
		break;

	case PT_FunctionCallArg:
		std::cout << "Function Call Arg: ";
		break;

	case PT_FunctionCallArgList:
		std::cout << "Function Call Arg List: ";
		break;

	default:
		break;
	}

	std::cout << std::endl;

	for (int i = 0; i < ParseTreeNode::MAX_CHILDREN; ++ i)
	{
		ParseTreeNode* child = node->getChild(i);
		if (child)
			print(child, indent + 2);
	}
}
// --------------------------------------------------------------------------------

}