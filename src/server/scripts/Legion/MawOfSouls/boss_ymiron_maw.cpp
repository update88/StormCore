/*
* Copyright (C) 2014-2017 StormCore
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "maw_of_souls.h"

enum Spells {

	// Ymiron Spells
	SPELL_BANE = 193460,
	SPELL_SLASH = 193211,
	SPELL_SCREAMS = 193364,
	SPELL_SIPHON = 194665,
	SPELL_BLOOD = 213738,
	SPELL_WINDS = 193977,
	SPELL_FALLEN = 193566,

	// Bane Orb Spells
	SPELL_BANE_NOVA = 200194,

	// Risen Warrior Spells
	SPELL_VIGOR_STACK = 203816
};

enum Auras {
	AURA_BANE_SELF = 193460
};

enum Adds {
	NPC_FALLEN_WARRIOR = 98246,
	NPC_BANE_SPHERE = 97099
};

enum Yells {
	YELL_PRE_ENCOUNTER = 0,
	YELL_ENTER_COMBAT = 1,
	YELL_BANE = 2,
	YELL_FALLEN = 3,
	YELL_SCREAMS = 4,
	YELL_WINDS = 5,
	YELL_KILL = 6,
	YELL_KILLED = 7
};

enum Events {
	EVENT_SLASH = 1,
	EVENT_SCREAMS = 2,
	EVENT_WINDS = 3,
	EVENT_BANE = 4,
	EVENT_FALLEN = 5,
	EVENT_CREATE_BANE_SPHERE = 6
};

Position const YmironPositions[2] = {
	{ 7407.954102f, 7265.254883f, 51.087051f, 5.549540f },
	{ 7395.062012f, 7275.660156f, 43.792096f, 5.467066f }
};

enum Misc {
	DATA_INSTANT_KARMA = 1
};

class boss_ymiron_maw : public CreatureScript {
	public:
		boss_ymiron_maw() : CreatureScript("boss_ymiron_maw") { }

		struct boss_ymiron_mawAI : public BossAI {

			boss_ymiron_mawAI(Creature* creature) : BossAI(creature, DATA_YMIRON_MAW) {
				instance = me->GetInstanceScript();
				getDifficuly();
				Initialize();
			}

			void Initialize() {
				events.ScheduleEvent(EVENT_SLASH, 3.5 * IN_MILLISECONDS);
				events.ScheduleEvent(EVENT_SCREAMS, 5.9 * IN_MILLISECONDS);
				events.ScheduleEvent(EVENT_WINDS, 15 * IN_MILLISECONDS);
				events.ScheduleEvent(EVENT_BANE, 21 * IN_MILLISECONDS);
				if(isHeroicOrMythic) {
					events.ScheduleEvent(EVENT_FALLEN, 39 * IN_MILLISECONDS);
				}
			}

			void Reset() override {
				BossAI::Reset();
				Initialize();
				events.Reset();
				instance->SetBossState(DATA_YMIRON_MAW, FAIL);
			}

			void EnterCombat(Unit* who) {
				BossAI::EnterCombat(who);
				Talk(YELL_ENTER_COMBAT);
				instance->SetBossState(DATA_YMIRON_MAW, IN_PROGRESS);
			}

			void JustDied(Unit* killer) override {
				BossAI::JustDied(killer);
				Talk(YELL_KILLED);
				instance->SetBossState(DATA_YMIRON_MAW, DONE);
			}

			void JustReachedHome() override {
				BossAI::JustReachedHome();
				me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 16);
			}

			void EnterEvadeMode(EvadeReason why) override {
				BossAI::EnterEvadeMode(why);
				instance->SetBossState(DATA_YMIRON_MAW, FAIL);
				me->SetHomePosition(YmironPositions[1]);
				me->CombatStop();
				me->RemoveAllAuras();
				events.Reset();
				Initialize();
			}

			void KilledUnit(Unit* victim) override {
				if(victim->GetTypeId() == TYPEID_PLAYER)
					Talk(YELL_KILL);
			}

			uint32 GetData(uint32 type) const override {
				if(type == DATA_INSTANT_KARMA)
					return instantKarma ? 1 : 0;
				return 0;
			}

			void MoveInLineOfSight(Unit* who) override {
				if(who->GetTypeId() == TYPEID_PLAYER && me->GetDistance2d(who) < 85.0f && !me->IsInCombat()) {
					//Talk(YELL_PRE_ENCOUNTER);
					// Should start the event here
				}
			}

			void SummonedCreatureDies(Creature* summoned, Unit* /*who*/) override {
				// Achievement Logic Missing
			}

			void UpdateAI(uint32 diff) override {
				if(!UpdateVictim())
					return;

				events.Update(diff);

				if(me->HasUnitState(UNIT_STATE_CASTING))
					return;

				if(me->GetDistance(me->GetHomePosition()) > 120.0f) {
					EnterEvadeMode(EVADE_REASON_BOUNDARY);
					return;
				}


				if(me->HasAura(AURA_BANE_SELF)) {
					if(createdSpheres <= allowedSpheres) {
						createdSpheres++;
						activeSpheres++;
						me->SummonCreature(NPC_BANE_SPHERE, me->GetPositionX() + getRandomFloat(-5.0f, 5.0f), me->GetPositionY() + getRandomFloat(-5.0f, 5.0f), me->GetPositionZ(), 2.0f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 15 * IN_MILLISECONDS);
					}
				}


				while(uint32 eventID = events.ExecuteEvent()) {
					switch(eventID) {

						case EVENT_SLASH:
							DoCastVictim(SPELL_SLASH);
							events.ScheduleEvent(EVENT_SLASH, 14.6 * IN_MILLISECONDS);
						break;

						case EVENT_SCREAMS:
							DoCastVictim(SPELL_SCREAMS);
							Talk(YELL_SCREAMS);
							events.ScheduleEvent(EVENT_SCREAMS, 23 * IN_MILLISECONDS);
						break;

						case EVENT_WINDS:
							DoCastVictim(SPELL_WINDS);
							Talk(YELL_WINDS);
							events.ScheduleEvent(EVENT_WINDS, 24 * IN_MILLISECONDS);
						break;

						case EVENT_BANE:
							DoCast(SPELL_BANE);
							Talk(YELL_BANE);
							events.ScheduleEvent(EVENT_BANE, 49.5 * IN_MILLISECONDS);
							if(isHeroicOrMythic) {
								events.ScheduleEvent(EVENT_FALLEN, 67.4 * IN_MILLISECONDS);
							}
						break;

						case EVENT_FALLEN:
							DoCast(SPELL_FALLEN);
							Talk(YELL_FALLEN);
							for(int sphere = 0; sphere < activeSpheres; sphere++) {
								if(Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true)) {
									Position playerPosition = target->GetPosition();
									me->SummonCreature(NPC_FALLEN_WARRIOR, playerPosition.m_positionX + getRandomFloat(-3.0f, 3.0f), playerPosition.m_positionY + getRandomFloat(-3.0f, 3.0f), playerPosition.m_positionZ, 2.0f, TEMPSUMMON_DEAD_DESPAWN);
								}
							}
							createdSpheres = 0;
							activeSpheres = 0;
						break;

						default:
							break;
					}
				}

				DoMeleeAttackIfReady();
			}

			void updateSpheresCount() {
				--activeSpheres;
			}

			void getDifficuly() {
				Difficulty instanceDifficulty = instance->instance->GetDifficultyID();
				uint64 myHealth = me->GetHealth();
				double healthModifier;
				double damageModifier;

				if(instanceDifficulty == DIFFICULTY_NORMAL) {
					healthModifier = 1.0;
					damageModifier = 1.0;
				} else if(instanceDifficulty == DIFFICULTY_HEROIC) {
					healthModifier = 1.0571428043; // HealthModifier 36.99
					damageModifier = 1.39285714286;
					isHeroicOrMythic = true;
				} else if(instanceDifficulty == DIFFICULTY_MYTHIC) {
					healthModifier = 1.32142855675; // HealthModifier 46.25
					damageModifier = 1.95;
					isHeroicOrMythic = true;
				} else {
					healthModifier = 20;
					damageModifier = 20;
				}
				uint64 difficultyAdjustedHealth = static_cast<unsigned long long>(myHealth * healthModifier);
				me->SetMaxHealth(difficultyAdjustedHealth);
				me->SetHealth(difficultyAdjustedHealth);
				me->SetModifierValue(UNIT_MOD_DAMAGE_MAINHAND, BASE_VALUE, static_cast<float>(damageModifier));
			}

			float getRandomFloat(float a, float b) {
				float random = ((float) rand()) / (float) RAND_MAX;
				float diff = b - a;
				float r = random * diff;
				return a + r;
			}

			private:
				InstanceScript* instance;
				bool instantKarma;
				bool isHeroicOrMythic = false;
				uint32 killedWarriors = 0;

				// Bane Spheres Variables
				uint32 createdSpheres = 0;
				uint32 activeSpheres = 0;
				uint32 allowedSpheres = 7;
		};

		CreatureAI* GetAI(Creature* creature) const override {
			return GetMawOfSoulsAI<boss_ymiron_mawAI>(creature);
		}
};

// TEMP NPC 97099
class npc_bane_sphere : public CreatureScript {
public:
	npc_bane_sphere() : CreatureScript("npc_bane_sphere") {}
	struct npc_bane_sphereAI : public ScriptedAI {
		npc_bane_sphereAI(Creature* creature) : ScriptedAI(creature) {
			instance = me->GetInstanceScript();
			Initialize();
		}

		void Initialize() {
			me->setFaction(84);
			me->SetSpeed(MOVE_RUN, 15.0f);
			if(Creature* boss = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_YMIRON_MAW)))
				me->GetMotionMaster()->MoveCirclePath(boss->GetPositionX() + getRandomFloat(-10.0f, 10.0f), boss->GetPositionY() + getRandomFloat(-10.0f, 10.0f), boss->GetPositionZ(), getRandomFloat(12.0f, 18.0f), true, 16);
		}

		void MoveInLineOfSight(Unit* who) override {
			if(who->GetTypeId() == TYPEID_PLAYER && me->GetDistance2d(who) < 2.3f) {
				DoCast(SPELL_BANE_NOVA);
				if(Creature* boss = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_YMIRON_MAW)))
					ENSURE_AI(boss_ymiron_maw::boss_ymiron_mawAI, boss->AI())->updateSpheresCount();
				me->DespawnOrUnsummon();
			}
		}

		void UpdateAI(uint32 /*diff*/) override { }

		float getRandomFloat(float a, float b) {
			float random = ((float) rand()) / (float) RAND_MAX;
			float diff = b - a;
			float r = random * diff;
			return a + r;
		}

		private:
			InstanceScript* instance;
	};

	CreatureAI* GetAI(Creature* creature) const override {
		return GetMawOfSoulsAI<npc_bane_sphereAI>(creature);
	}
};

// NPC 98246
class npc_bane_skeleton : public CreatureScript {
	public:
		npc_bane_skeleton() : CreatureScript("npc_bane_skeleton") { }
		struct npc_bane_skeletonAI : public ScriptedAI {
			npc_bane_skeletonAI(Creature* creature) : ScriptedAI(creature) {
				instance = me->GetInstanceScript();
				Initialize();
			}

			void Initialize() {
				DoCast(me, SPELL_VIGOR_STACK);
				vigorStackTimer = _vigorStackTimer;
			}

			void JustDied(Unit* killer) override {
				// Handle mechanics when add killed by another one (for the achievement)
			}

			void UpdateAI(uint32 diff) override {
				if(!UpdateVictim())
					return;

				if(vigorStackTimer <= diff) {
					DoCast(me, SPELL_VIGOR_STACK);
					vigorStackTimer = _vigorStackTimer;
				} else vigorStackTimer -= diff;
				
				DoMeleeAttackIfReady();
			}

			private:
				InstanceScript* instance;
				uint32 vigorStackTimer;
				uint32 _vigorStackTimer = 6 * IN_MILLISECONDS;
		};

		CreatureAI* GetAI(Creature* creature) const override {
			return GetMawOfSoulsAI<npc_bane_skeletonAI>(creature);
		}
};

void AddSC_boss_ymiron_maw() {
	new boss_ymiron_maw();
	new npc_bane_skeleton();
	new npc_bane_sphere();
	//new achievement_instant_karma();
}
