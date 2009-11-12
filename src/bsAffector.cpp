#include <iostream>
#include "bsAffector.h"
#include "bsScriptMachine.h"

namespace BS_NMSP
{

// --------------------------------------------------------------------------------
Affector::Affector(const String& name, ScriptMachine* machine, AffectorFunction func, 
				   int numArgs, const BytecodeBlock& code) : 
	mName(name),
	mFunction(func),
	mbRecalculate(true),
	mbRecalculateAlways(false),
	mScriptMachine(machine),
	mBytecode(0),
	mBytecodeSize(0)
{
	if (!code.empty())
	{
		mBytecodeSize = code.size();
		mBytecode = new uint32[mBytecodeSize];
		for (size_t i = 0; i < mBytecodeSize; ++i)
			mBytecode[i] = code[i];
	}
}
// --------------------------------------------------------------------------------
Affector::~Affector()
{
	delete[] mBytecode;
}
// --------------------------------------------------------------------------------
const String& Affector::getName() const
{
	return mName;
}
// --------------------------------------------------------------------------------
void Affector::recalculateArguments()
{
	mState.curInstruction = 0;
	mState.stackHead = 0;

	mScriptMachine->interpretCode(mBytecode, mBytecodeSize, mState);

	if (!mbRecalculateAlways)
		mbRecalculate = false;
}
// --------------------------------------------------------------------------------
void Affector::recalculateAlways(bool always)
{
	mbRecalculateAlways = always;
}
// --------------------------------------------------------------------------------
void Affector::execute(UserTypeBase* object, float frameTime)
{
	if (mbRecalculate)
		recalculateArguments();

	mFunction(object, frameTime, mState.stack + mState.stackHead);
}
// --------------------------------------------------------------------------------
void Affector::onChanged()
{
	mbRecalculate = true;
}
// --------------------------------------------------------------------------------

}
