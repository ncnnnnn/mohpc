#include <Shared.h>
#include <MOHPC/Managers/EmitterManager.h>
#include <MOHPC/Managers/EventManager.h>
#include <MOHPC/Managers/ShaderManager.h>
#include <MOHPC/Formats/TIKI.h>
#include "../Misc/EmitterListener.h"

using namespace MOHPC;

CLASS_DEFINITION(EmitterManager);
EmitterManager::EmitterManager()
{
}

Sprite::Sprite()
{
	spriteType = ST_None;
	pointerValue = nullptr;
}

MOHPC::Sprite::Sprite(const Sprite& sprite)
{
	*this = sprite;
}

MOHPC::Sprite& MOHPC::Sprite::operator=(const Sprite& sprite)
{
	spriteType = sprite.spriteType;
	pointerValue = sprite.pointerValue;

	if (pointerValue)
	{
		if (spriteType == ST_Tiki)
		{
			Tiki = new SharedPtr<class TIKI>(*sprite.Tiki);
		}
		else if (spriteType == ST_Shader)
		{
			Shader = new ShaderPtr(*sprite.Shader);
		}
	}

	return *this;
}

MOHPC::Sprite::Sprite(Sprite&& sprite)
{
	spriteType = sprite.spriteType;
	pointerValue = sprite.pointerValue;
	sprite.spriteType = ST_None;
	sprite.pointerValue = nullptr;
}

Sprite::~Sprite()
{
	if (pointerValue)
	{
		if (spriteType == ST_Tiki)
		{
			delete Tiki;
		}
		else if (spriteType == ST_Shader)
		{
			delete Shader;
		}
		pointerValue = nullptr;
	}
}

Emitter::Emitter()
{
	bIsEmitter = false;
	flags = (emitterFlags_e)0;
	spawnType = EST_None;
	count = 1;
	life = 0.f;
	randomLife = 0.f;
	startTime = 0.f;
	color = Vector(1.f, 1.f, 1.f);
	alpha = 1.f;
	scaleRate = 0.f;
	scaleMin = 0.f;
	scaleMax = 0.f;
	scale = 1.f;
	fadeInTime = 0.f;
	fadeDelay = 0.f;
	sphereRadius = 0.f;
	coneHeight = 0.f;
	spawnRate = 0.f;
	forwardVelocity = 0.f;
	lightRadius = 0.f;
}

const char* MOHPC::Emitter::GetEmitterName() const
{
	return emitterName.c_str();
}

EmitterResults::EmitterResults()
{
	animNum = -1;
}

size_t MOHPC::EmitterResults::GetNumEmitters() const
{
	return Emitters.size();
}

const MOHPC::Emitter* MOHPC::EmitterResults::GetEmitterAt(size_t Index) const
{
	return Index < Emitters.size() ? &Emitters[Index] : nullptr;
}

const char* MOHPC::EmitterResults::GetAnimName() const
{
	return animName.c_str();
}

bool EmitterManager::ParseEmitters(const TIKI* Tiki, EmitterResults& Results)
{
	size_t cursor = 0;

	uint32_t animNum = Results.animNum;
	Results = EmitterResults();

	bool bProcessedCommands = true;
	bool bAnimHasCommands = false;
	while(bProcessedCommands || !bAnimHasCommands)
	{
		Emitter Emitter;
		EmitterListener listener(&Emitter);
		listener.InitAssetManager(GetAssetManager());

		bProcessedCommands = false;

		if (animNum == -1)
		{
			// Parse init commands

			const size_t numCmds = Tiki->GetNumClientInitCommands();
			for (; cursor < numCmds; cursor++)
			{
				const TIKIAnim::Command* cmd = Tiki->GetClientInitCommand(cursor);

				if(ProcessCommand(cmd->args, listener))
				{
					bProcessedCommands = true;
					bAnimHasCommands = true;
					cursor++;
					break;
				}
			}

			if (cursor == numCmds)
			{
				Results.animName = "";
			}
		}
		else if (animNum < Tiki->GetNumAnimations())
		{
			const TIKIAnim::AnimDef* animDef = Tiki->GetAnimDef(animNum);

			// Parse animation commands
			const size_t numCmds = animDef->client_cmds.size();
			for (; cursor < numCmds; cursor++)
			{
				const TIKIAnim::Command* cmd = &animDef->client_cmds[cursor];

				if (cmd->args.size())
				{
					if (ProcessCommand(cmd->args, listener))
					{
						bProcessedCommands = true;
						bAnimHasCommands = true;
						cursor++;
						break;
					}
				}
			}

			if (cursor == numCmds)
			{
				Results.animName = animDef->alias;
			}
		}
		else
		{
			break;
		}

		if (bProcessedCommands)
		{
			Results.Emitters.push_back(std::move(Emitter));
		}
		else if (!bAnimHasCommands)
		{
			animNum++;
		}
	}

	cursor = 0;
	Results.animNum = animNum + 1;

	return bAnimHasCommands;
}

bool EmitterManager::ProcessCommand(const Container<str>& Arguments, EmitterListener& Listener)
{
	Event* ev = GetManager<EventManager>()->NewEvent(Arguments[0].c_str());

	const size_t numArgs = Arguments.size();
	for (size_t j = 1; j < numArgs; j++)
	{
		ev->AddToken(Arguments[j].c_str());
	}

	Listener.ProcessEvent(ev);

	if (Listener.FinishedParsing())
	{
		return true;
	}

	return false;
}
