#ifndef __BS_SCRIPTMACHINE_H__
#define __BS_SCRIPTMACHINE_H__

#include <vector>
#include <map>
#include <list>
#include "bsPrerequisites.h"
#include "bsCore.h"
#include "bsScriptVariables.h"
#include "bsFireType.h"
#include "bsEmitter.h"
#include "bsEmitterDefinition.h"
#include "bsDeepMemoryPool.h"
#include "bsLog.h"

namespace BS_NMSP
{
	class TypeManager;

	class ScriptMachine
	{
		friend class Machine;
		friend class ParseTree;

		// TypeManager
		TypeManager* mTypeManager;

		// Log
		Log* mLog;

		// Native functions callable by script
		struct NativeFunctionRecord
		{
			String name;
			NativeFunction function;
		};

		std::vector<NativeFunctionRecord> mNativeFunctions;

		// Global variables
		std::vector<GlobalVariable*> mGlobals;

		// Member variable declarations
		MemberVariableDeclarationMap mMemberVariableDeclarations;

		// Emitter Definitions and pools
		struct EmitterRecord
		{
			String name;
			EmitterDefinition* def;
			DeepMemoryPool<FireTypeControl, int>* pool;
		};
		
		typedef std::vector<EmitterRecord> EmitterRecordList;
		EmitterRecordList mEmitterRecords;

		DeepMemoryPool<Emitter, ScriptMachine*>* mEmitters;

		// Global property list
		std::vector<String> mProperties;

		// CodeRecords
		typedef std::vector<CodeRecord*> CodeList;
		CodeList mCodeRecords;

	private:

		bool checkInstructionPosition(ScriptState& st, size_t length, bool loop);

		void setTypeManager(TypeManager* typeMan);

	protected:

		// Can only be created by Machine
		ScriptMachine(Log* _log);

	public:

		~ScriptMachine();

		// Emitters
		Emitter* createEmitter(const String& definition);

		void destroyEmitter(Emitter* emit);

		void updateEmitters(float frameTime);

		// CodeRecords
		void createCodeRecord();

		CodeRecord* getCodeRecord(int index);

		int getNumCodeRecords() const;

		// FireTypeScriptRecords
		FireTypeControl* getFireTypeRecord(int index);

		void releaseFireTypeRecord(int index, FireTypeControl* rec);

		// Native functions
		void registerNativeFunction(const String& name, NativeFunction func);

		int getNativeFunctionIndex(const String &name) const;

		NativeFunction getNativeFunction(int index) const;

		// Fire types
		FireType* getFireType(const String& name) const;

		// Properties
		void addProperty(const String& prop);

		int getPropertyIndex(const String& prop) const;

		const String& getProperty(int index) const;

		// Global variables
		void registerGlobalVariable(const String& name, bstype initialValue);

		int getGlobalVariableIndex(const String& name) const;

		bstype getGlobalVariableValue(int index) const;

		void setGlobalVariableValue(const String& name, bstype value);

		GlobalVariable *getGlobalVariable(const String& name);

		GlobalVariable *getGlobalVariable(int index);

		// Emitter Definitions
		bool addEmitterDefinition(const String &name, EmitterDefinition* def);

		EmitterDefinition* getEmitterDefinition(const String &name) const;

		int getNumEmitterDefinitions() const;

		// Script state processing
		void interpretCode(const uint32* code, size_t length, ScriptState& st, int* curState, 
			FireTypeControl* record, bstype x, bstype y, bstype* members, bool loop);

		void processEmitterState(EmitterScriptRecord* gsr);

		void processConstantExpression(const uint32* code, size_t length, EmitterScriptRecord* gsr);

		// Compilation
		int compileScript(uint8* buffer, size_t bufferSize);

		void declareMemberVariable(const String& emit, const String& var, bstype value);

		// Errors
		void addErrorMsg (const String& msg);
	};

}

#endif