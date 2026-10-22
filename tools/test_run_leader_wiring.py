"""The run leader's glue hands the policy what its retry rules match on.

runtime/DungeonRunPolicy.h retries a refused gather once and passes a refused
walk-back point by comparing the refused order's text with the order it sent
that tick (issue #999), so the glue in commands/BotCommands.cpp must report a
refusal with the order exactly as Tick() returned it. A dead lead bot released
by the regroup's `revive party` is a ghost off the plan's map; the glue must
not pause the run for that while a regroup (a resume or one the run started by
itself after a wipe) is on. `cmd move` must judge the point with
RemoteCommander::CheckMove from the lead bot's own 3D distance, which is what
lets the run leader's gather on the lead bot's own spot through (and only that
gather, not a player's order). During a walk back the step facts (hop pulls)
are read for the replayed move point.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class RunLeaderWiringTest(unittest.TestCase):
    def test_refusal_reports_the_order_as_sent(self):
        self.assertIn("std::string const order = TortoiseBots::DungeonRun::Tick(slot.run, facts, dt);", COMMANDS)
        self.assertIn("bool const accepted = IssueRunOrder(commander, order, answer,\n"
                      "            TortoiseBots::DungeonRun::IsGatherOrder(it->second.run, order));", COMMANDS)
        self.assertIn("TortoiseBots::DungeonRun::OrderRefused(it->second.run, order, answer", COMMANDS)

    def test_ghost_lead_tolerated_during_a_regroup(self):
        # Any dead lead bot off the map (a ghost after a wipe too) lets the tick run; it starts the regroup.
        self.assertIn("bool const deadLeadAway = !lead->IsAlive();", COMMANDS)
        self.assertIn("if (lead->GetMapId() != slot.run.plan->map && !deadLeadAway)", COMMANDS)

    def test_exact_waypoint_facts_and_kind(self):
        # The exact waypoint's stand-still proof reads the lead bot's moving state (DungeonRunPolicy.h ExactWalkTo), and the
        # loader reads a move_hop_exact row as a MoveDirect with exactLaunch (DungeonPlanPolicy.h ParsePlanStepRowKind).
        self.assertIn("f.leadMoving = lead->IsMoving();", body(COMMANDS, "void ReadRunFacts(", "\n}\n"))
        loader = (ROOT / "ai/playerbot/DungeonPlanMgr.cpp").read_text()
        self.assertIn("ParsePlanStepRowKind(fields[3].GetCppString(), step.kind, step.exactLaunch);", loader)
        self.assertNotIn("step.kind = ParsePlanStepKind(", loader)

    def test_cmd_move_checks_the_lead_distance(self):
        self.assertIn("facts.distanceFromLead = std::sqrt(dx * dx + dy * dy + dz * dz);", COMMANDS)
        self.assertIn("RemoteCommander::MoveVerdict verdict = RemoteCommander::CheckMove(facts);", COMMANDS)

    def test_only_the_run_leaders_gather_gets_the_lead_spot_pass(self):
        # A player's `cmd move` never sets it: only IssueRunOrder, for the run's own gather.
        self.assertIn("facts.leadSpotGather = s_runIssuing && s_runGather;", COMMANDS)
        issue = body(COMMANDS, "bool IssueRunOrder(", "\n}\n")
        self.assertIn("s_runGather = gather;", issue)
        self.assertIn("s_runGather = false;", issue)
        self.assertEqual(COMMANDS.count("s_runGather = gather;"), 1)

    def test_gather_stragglers_are_reported_by_name(self):
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertIn("member.name = bot->GetName();", facts)
        self.assertIn("member.yd = yd;", facts)
        self.assertIn("member.dz = bot->GetMap() == lead->GetMap() ? std::fabs(bot->GetPositionZ() - lead->GetPositionZ()) : 0.0f;", facts)
        # During a walk back out of sight the hop pulls are read around the next approach hop.
        self.assertIn("cx = f.approachX;", facts)
        self.assertIn("f.members.push_back(member);", facts)

    def test_run_hold_is_let_go_on_every_path(self):
        # The hold (DungeonRun::Run::hold) is synced after every tick that runs, and let go on the
        # paths that return before Tick: a paused run, the lead bot off the plan's map, `run stop`.
        tick = body(COMMANDS, "void TickDungeonRun(", "\n}\n")
        self.assertIn("SyncRunHold(commander, lead, bots, it->second, !order.empty());", tick)
        early = body(tick, "if (slot.run.state != TortoiseBots::DungeonRun::State::Running || !commander)", "return;")
        self.assertIn("ReleaseRunHold(commander, binding->leadGuid, slot);", early)
        offmap = body(tick, "the lead bot is not on the plan's map", "return;")
        self.assertIn("ReleaseRunHold(commander, binding->leadGuid, slot);", offmap)
        stop = body(COMMANDS, 'slot.run.note = RunStopNote(rawArg);', "DungeonRuns().erase(it);")
        self.assertIn("ReleaseRunHold(commander, binding->leadGuid, slot);", stop)
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertIn("f.deadNames.push_back(bot->GetName());", facts)
        self.assertIn("member.x = bot->GetPositionX();", facts)

    def test_run_back_reads_cached_attackers_and_restores_the_ai(self):
        # A member coming back with attackers (DungeonRun::RunBack): its attackers are counted from the
        # core's attacker set (no scan); the glue makes a listed bot a passive follower of the lead bot,
        # never undoes a passive stance it did not set, and gives the AI back once the run lets it go.
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertIn("for (Unit* attacker : bot->GetAttackers())", facts)
        self.assertIn("++member.attackers;", facts)
        start = body(COMMANDS, "void StartRunBackPassive(", "\n}\n")
        self.assertIn('ai->HasStrategy("passive", BotState::BOT_STATE_COMBAT)', start)
        self.assertIn('ai->ChangeStrategy("+passive", BotState::BOT_STATE_COMBAT);', start)
        self.assertIn("PointFollowerAtLead(commanderRaw, bot, lead);", start)
        tick = body(COMMANDS, "void TickDungeonRun(", "\n}\n")
        self.assertIn("for (std::string const& name : slot.run.runBack)", tick)
        sync = body(COMMANDS, "void SyncRunBackPassive()\n{", "\n}\n")
        self.assertIn("run->second.run.state == TortoiseBots::DungeonRun::State::Running", sync)
        self.assertIn("EndRunBackPassive(it->first);", sync)
        update = body(COMMANDS, "static void UpdateDungeonRuns(", "\n}\n")
        self.assertIn("SyncRunBackPassive();", update)
        self.assertIn("f.leadInCombat = member.inCombat;", facts)

    def test_player_order_restores_every_run_back_bot_first(self):
        # A player's order (a `cmd flee Bob` too) gives every bot the run made passive its own AI back
        # before the order runs; the order is noted before it executes at every call site, so a flee
        # then sets the stance on the bots it names only. No bot of the commander stays passive.
        pause = body(COMMANDS, "static void PauseDungeonRunForPlayer(uint64 commanderRaw, std::string const& order)\n{", "\n}\n")
        self.assertIn("EndRunBackPassive(passive->first);", pause)
        self.assertNotIn('order.find("flee")', pause)
        # The order is noted before it runs: right after parsing in `.bot action`.
        action = body(COMMANDS, "static bool HandleAction(ChatHandler* handler, char const* args)", "\n}\n")
        self.assertIn("if (parsed)\n        PauseDungeonRunForPlayer(", action)


    def test_boss_down_needs_his_corpse(self):
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertIn("if (bossEntry && fromPoint <= step->radius + kSeeYd)\n"
                      "                            f.bossDeadNearSpot = true;", facts)
        self.assertIn("if (bossEntry && lead->GetDistance(creature) <= kSeeYd)\n"
                      "                            f.bossDeadNearLead = true;", facts)

    def test_boss_corpse_guid_is_read_for_the_loot_step(self):
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertIn("f.bossCorpseGuid = creature->GetObjectGuid().GetRawValue();", facts)
        self.assertIn("(!f.bossCorpseGuid || fromPoint < bossCorpseFromSpot)", facts)

    def test_gather_anchor_path_is_read(self):
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        # Read lazily: only after the policy asked for it.
        self.assertIn("if (slot.run.anchorPathWanted && slot.run.anchorKnown && lead->IsAlive() && lead->IsInWorld() &&", facts)
        self.assertIn("f.anchorPathRead = true;", facts)
        self.assertIn("f.anchorReachable = !(type & RemoteCommander::kPathNoPath) &&", facts)

    def test_direct_walk_back_checks_the_path_against_packs(self):
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertIn("f.directPathCrossesPack = PathCrossesUnclearedPack(*slot.run.plan, slot.run.step, whole, f.team,\n"
                      "                            slot.run.notCleared);", facts)

    def test_crowd_control_and_kill_first_facts(self):
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        # The crowd control is a mage that knows Polymorph (the policy picks it from the members).
        self.assertIn("member.polymorph = bot->GetClass() == CLASS_MAGE && KnowsPolymorph(bot);", facts)
        self.assertIn("unit.polymorphed = Polymorphed(creature);", facts)
        # Kill-first units come from the bots' cached attackers lists: no grid or world scan (AGENTS.md).
        self.assertIn("ReadKillFirst(*slot.run.plan, lead, bots, f);", facts)
        kill = body(COMMANDS, "void ReadKillFirst(", "\n}\n")
        self.assertIn('ReadCachedGuidList(PlayerbotAIStorage::Instance().GetAI(bot), "attackers", guids);', kill)
        for scan in ("GetCreatureListWithEntryInGrid", "Cell::Visit", "VisitAllObjects", "HostileCreaturesAround"):
            self.assertNotIn(scan, kill)
        self.assertIn("fact.engaged = EngagedWithParty(unit, party);", kill)
        # A ward is read off its engaged owner at once (totems and guardians), not when a cached list takes it in.
        self.assertIn("CONTROLLED_TOTEMS | CONTROLLED_GUARDIANS", kill)
        self.assertIn("EngagedWithParty(summoner, party);", kill)
        loader = (ROOT / "ai/playerbot/DungeonPlanMgr.cpp").read_text()
        self.assertIn("SELECT `plan`, `map`, `cc_priority`, `cc_always`, `kill_first` FROM `ai_dungeon_plan`", loader)
        self.assertIn("table.SetUnits(plan, fields[2].GetCppString(), fields[3].GetCppString(), fields[4].GetCppString())", loader)

    def test_crowd_control_keeps_to_the_runs_mark(self):
        cc = (ROOT / "ai/playerbot/strategy/values/CcTargetValue.cpp").read_text()
        # Never a totem or an NPC's summon (the healing ward), and in a run only the ordered mark.
        self.assertIn("bool const totem = static_cast<Creature*>(creature)->IsTotem();", cc)
        self.assertIn("TortoiseBots::BotCommands::IsRunKillFirst(bot, creature->GetEntry());", cc)
        self.assertIn("if (TortoiseBots::CcLeavesOut(totem, npcSummoned, inRun, runKillFirst))", cc)
        self.assertIn("inRun = TortoiseBots::BotCommands::IsInRunningDungeonRun(ai->GetBot());", cc)
        self.assertIn("if (inRun && !assignedTarget)\n            return;", cc)
        query = body(COMMANDS, "RunningRunOf(Player* bot)", "\n}\n")
        self.assertIn("State::Running", query)

    def test_spells_at_a_totem_must_hit_it(self):
        spells = (ROOT / "ai/playerbot/strategy/actions/GenericSpellActions.cpp").read_text()
        useful = body(spells, "bool CastSpellAction::isUseful()", "\n}\n")
        self.assertIn("static_cast<Creature*>(spellTarget)->IsTotem()", useful)
        self.assertIn("TortoiseBots::SpellWorksOnTotem(info->Effect, info->EffectApplyAuraName,", useful)
        self.assertIn("info->EffectImplicitTargetA, MAX_EFFECT_INDEX, info->IsPositiveSpell()))", useful)

    def test_walk_back_reads_facts_for_the_replayed_point(self):
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertIn("ai::PlanStep const* step = FactStep(slot.run);", facts)
        self.assertIn("ai::OwnedByLaterStep(*slot.run.plan, OwnerFrom(slot.run), unit->GetEntry(),", facts)
        self.assertNotIn("Current(slot.run)", facts)

    def test_ritual_answer_reaches_the_run(self):
        # The run's `ritual` step waits for the ritual order's one answer (#984): the group's reply
        # notes it for the run whose carrier (the lead bot) is a member, and a run's `ritual` order
        # sets the run's carrier like loot/open/use (only the ritual's carrier is always the lead bot).
        reply = body(COMMANDS, "void RitualReply(RitualGroup const& group", "\n}\n")
        self.assertIn("run->second.taskReply = ok ? 1 : 2;", reply)
        self.assertIn('order.compare(0, 7, "ritual ") == 0', COMMANDS)
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertIn("case ai::PlanStepKind::Ritual:", facts)
        self.assertIn("lead->GetMap()->GetInstanceData()", facts)


    def test_open_use_carrier_is_pinned_while_its_order_is_out(self):
        # An open / use_item order's bot stays the carrier until the step ends: the facts of the
        # next ticks (busy, reply, the made item) are read from that bot, not re-picked from the
        # bags (Uldaman: the use consumes 7740/7741 in a non-lead bot's bags, PR #1078 review).
        task = body(COMMANDS, "Player* RunTaskBot(", "\n}\n")
        bags = body(COMMANDS, "std::vector<TortoiseBots::DungeonRun::BagFact> RunTaskBags(", "\n}\n")
        self.assertIn("bag.guid = bot->GetObjectGuid().GetRawValue();", bags)
        self.assertIn("OutstandingCarrierVerdict(slot.run.orderSent, slot.carrierRaw, bags, &name);", task)
        self.assertLess(task.index("OutstandingCarrierVerdict("), task.index("PickItemCarrier("))
        # Only with no order out is the carrier picked from the bags.
        self.assertIn("if (verdict == TortoiseBots::DungeonRun::CarrierVerdict::NoOrder)\n"
                      "        name = TortoiseBots::DungeonRun::PickItemCarrier(", task)
        # An order whose bot is gone is retired (orderSent and carrierRaw cleared) before the facts are read.
        retire = body(COMMANDS, "void RetireLostCarrier(", "\n}\n")
        self.assertIn("!= TortoiseBots::DungeonRun::CarrierVerdict::Retire)\n        return;", retire)
        self.assertIn("TortoiseBots::DungeonRun::RetireOrder(slot.run);", retire)
        self.assertIn("slot.carrierRaw = 0;", retire)
        self.assertIn("slot.taskReply = 0;", retire)
        tick = body(COMMANDS, "void TickDungeonRun(uint64 commanderRaw, uint32 dt)", "\n}\n")
        self.assertIn("RetireLostCarrier(slot, lead, bots);\n    TortoiseBots::DungeonRun::Facts facts;\n"
                      "    ReadRunFacts(slot, lead, bots, facts);", tick)
        policy = (ROOT / "runtime/DungeonRunPolicy.h").read_text()
        retire_order = body(policy, "inline void RetireOrder(Run& r)", "\n}\n")
        self.assertIn("r.orderSent = false;", retire_order)
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertIn("Player* const carrier = RunTaskBot(slot, lead, bots, step, &f.itemsSplit);", facts)
        # A new order picks afresh: the previous order's bot is cleared before the pick.
        self.assertIn("slot.carrierRaw = 0;\n            slot.carrierRaw = RunTaskBot(slot, lead, bots,", COMMANDS)

    def test_use_on_is_wired_like_an_open(self):
        # use_on (Zul'Farrak's gong with the Mallet): its carrier is the bot that carries the step's `item`, picked, pinned and
        # retired as for an open, and its facts (carrier, item, busy, answer) are read in the open / use_item case. The order
        # it sends starts with "use ", so the dispatch picks the carrier afresh for it. The policy dispatches the kind.
        bags = body(COMMANDS, "std::vector<TortoiseBots::DungeonRun::BagFact> RunTaskBags(", "\n}\n")
        self.assertIn("step.kind == ai::PlanStepKind::Open || step.kind == ai::PlanStepKind::UseOn ? step.item : step.entry", bags)
        task = body(COMMANDS, "Player* RunTaskBot(", "\n}\n")
        self.assertIn("step->kind != ai::PlanStepKind::UseOn)\n        return lead;", task)
        retire = body(COMMANDS, "void RetireLostCarrier(", "\n}\n")
        self.assertIn("step->kind != ai::PlanStepKind::UseOn))\n        return;", retire)
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertIn("case ai::PlanStepKind::UseItem:\n        case ai::PlanStepKind::UseOn:", facts)
        self.assertIn("f.carrierHasItem = step->item && carrier->HasItemCount(step->item);", facts)
        self.assertIn('order.compare(0, 4, "use ") == 0', COMMANDS)
        policy = (ROOT / "runtime/DungeonRunPolicy.h").read_text()
        self.assertIn("case PlanStepKind::UseOn:\n            UseOnStep(r, f, *s, order);", policy)
        # The review of 2026-10-21: the answer carries whether the act had started and whether the object reacted (its state
        # before the cast is kept for a use on a game object), the slot clears both with each new order, and the facts read the
        # summoned boss (the boss row after the use_on) alive or dead around the lead bot.
        note = body(COMMANDS, "void NoteRunTaskReply(CommanderTask const& task, bool ok, std::string const& text)", "\n}\n")
        self.assertIn("it->second.taskActStarted = task.actStarted;", note)
        self.assertIn("ObjectEffectOf(task, ai) == RemoteCommander::ObjectEffect::Reacted", note)
        begin = body(COMMANDS, "ActStart BeginUseAct(", "\n}\n")
        self.assertLess(begin.index("task.baseline = PackGoState(go);"), begin.index('ExecuteQuietAction(ai, "commander use"'))
        self.assertIn("slot.taskActStarted = false;\n            slot.taskObjectReacted = false;", COMMANDS)
        self.assertIn("slot.taskActStarted = false;\n    slot.taskObjectReacted = false;", retire)
        self.assertIn("f.taskActStarted = slot.taskActStarted;", facts)
        self.assertIn("f.objectReacted = slot.taskObjectReacted;", facts)
        self.assertIn("f.summonSeen = !found.empty();", facts)
        use_on = body(policy, "inline void UseOnStep(", "\n}\n")
        self.assertIn("TaskResendable(f.taskText) && !f.taskActStarted && r.taskResends < kUseOnResends", use_on)
        reset = body(policy, "inline void ResetStep(Run& r)", "\n}\n")
        self.assertNotIn("useOnRang", reset)
        self.assertIn('order = "use " + std::to_string(s.item) + " " + f.carrier + " " + EntryArg(s.entry, s.x, s.y, s.z);', use_on)

    def test_live_fixes_2026_10_16_are_wired(self):
        # 1. `cmd move` takes the floor at or below the point before a surface above it (a low ceiling).
        move = body(COMMANDS, "static bool CommanderMove(", "\n}\n")
        self.assertIn("RemoteCommander::ChooseFloorZ(point.z, belowZ,", move)
        # 2. A follower off the lead bot's line walks its own; the refusal still names the bot for the summon.
        direct = body(COMMANDS, "static bool CommanderMoveDirect(ChatHandler* handler, CommanderOrder const& order,\n"
                      "    RemoteCommander::MovePoint const& point, std::string const& intent)\n{", "\n}\n")
        join = body(COMMANDS, "static RemoteCommander::DirectJoin DirectFollowerJoin(", "\n}\n")
        self.assertIn("pathComplete, pathLength, ownLine);", join)
        self.assertIn("DirectFollowerJoin(bot, map, leadX, leadY, leadZ, point.x, point.y, ground, horizontal, route);",
                      direct)
        self.assertIn("join == RemoteCommander::DirectJoin::Own", direct)
        self.assertIn('" is not with " + lead->GetName()', direct)

    def test_direct_gate_asks_the_steps_own_test(self):
        # r12-19z29 Uldaman leg 5: the gate before a direct step passed on the 20 yd ground distance while the step
        # refused the bot too-far. The facts read the step's own test (DirectFollowerJoin) per follower while a
        # direct step is under the gate, and the policy's gate asks it.
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertIn("DirectJoinWanted(slot.run, joinX, joinY, joinZ);", facts)
        self.assertIn("f.directJoinRead = joinWanted;", facts)
        self.assertIn("f.directJoinSupported = true;", facts)
        # Cost: no path for a follower past kGatherYd (its distance fails the gate already); none during a walk
        # (DirectJoinWanted is false in Phase::Walk: test_dungeon_run_policy TestDirectOrderWaitsForTheRead).
        self.assertIn("if (joinWanted && bot != lead && yd <= kGatherYd && bot->IsInWorld()", facts)
        self.assertIn("member.withLead = DirectFollowerJoin(bot, joinMap, f.leadX, f.leadY, f.leadZ, joinX, joinY, "
                      "joinGround,", facts)
        self.assertLess(facts.index("member.withLead = DirectFollowerJoin("), facts.index("f.members.push_back(member);"))
        policy = (ROOT / "runtime/DungeonRunPolicy.h").read_text()
        move = body(policy, "inline void MoveStep(", "\n}\n")
        self.assertIn("s.kind == PlanStepKind::MoveDirect && r.phase != Phase::Walk && !DirectGathered(f)", move)
        self.assertIn('yd from the lead bot)", true);', move)
        walk_back = body(policy, "inline bool WalkBack(", "\n}\n")
        self.assertIn("if (direct && r.phase != Phase::Walk && HoldForDirectJoin(r, f))", walk_back)
        self.assertIn("if (s.kind == PlanStepKind::MoveDirect && r.phase != Phase::Walk && HoldForDirectJoin(r, f))", move)
        self.assertIn("direct && r.phase != Phase::Walk && !DirectGathered(f)", walk_back)
        self.assertIn('yd from the lead bot)", true);', walk_back)
        # 4. A door whose lock needs no key and no skill opens on a plain use.
        use = (ROOT / "ai/playerbot/strategy/actions/UseItemAction.cpp").read_text()
        self.assertIn("TortoiseBots::DoorLock::OpensOnPlainUse(lockTypes, lockIndex)", use)
    def test_uldaman_run3_hold_fixes_are_wired(self):
        # A hold's done condition is read on every tick, before the facts' own step (in a walk back too) ...
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertLess(facts.index("ReadHoldDone(slot.run.plan->steps[slot.run.step], lead, f);"),
                        facts.index("ai::PlanStep const* step = FactStep(slot.run);"))
        # ... and while the run is paused, before the paused early return.
        tick = body(COMMANDS, "void TickDungeonRun(", "\n}\n")
        self.assertLess(tick.index("TortoiseBots::DungeonRun::HoldDoneWhilePaused(slot.run, doneFacts)"),
                        tick.index("if (slot.run.state != TortoiseBots::DungeonRun::State::Running || !commander)"))
        # The party's places on a hold spot follow every tick and are let go with the run's hold.
        self.assertIn("SyncHoldStands(commander, lead, bots, it->second, order);", tick)
        release = body(COMMANDS, "void ReleaseRunHold(", "\n}\n")
        self.assertIn("slot.placedRaw.clear();", release)
        stands = body(COMMANDS, "void SyncHoldStands(", "\n}\n")
        self.assertIn("if (stand && (!placed || newPlaces || drifted))", stands)
        pause = body(COMMANDS, 'if (verb == "pause")', "else if")
        self.assertIn('PausePlayer(slot.run, "paused by .bot run pause");', pause)
        self.assertIn("member.ranged = PlayerbotAI::IsHeal(bot)", facts)
        self.assertIn("unit.x = creature->GetPositionX();", facts)


    def test_small_batch_2026_10_17_is_wired(self):
        # 1. `cmd move` hands the policy how far the path's end is from the lead bot (an incomplete path that goes nowhere).
        move = body(COMMANDS, "static bool CommanderMove(", "\n}\n")
        self.assertIn("facts.endFromLead = std::sqrt(lx * lx + ly * ly + lz * lz);", move)
        self.assertIn("facts.fromRunLeader = s_runIssuing;", move)
        # 2. The run's time limit ends the run with its own note; `run stop` takes a reason.
        tick = body(COMMANDS, "void TickDungeonRun(", "\n}\n")
        self.assertIn("RunTimeLimitReached(slot.elapsedMs, sPlayerbotAIConfig.dungeonRunMaxMinutes)", tick)
        self.assertIn("RunTimeLimitNote(sPlayerbotAIConfig.dungeonRunMaxMinutes)", tick)
        self.assertIn("slot.run.note = RunStopNote(rawArg);", COMMANDS)
        config = (ROOT / "ai/playerbot/PlayerbotAIConfig.cpp").read_text()
        template = (ROOT / "ai/playerbot/aiplayerbot.conf.dist.in").read_text()
        self.assertIn('"AiPlayerbot.DungeonRunMaxMinutes", 180', config)
        self.assertIn("AiPlayerbot.DungeonRunMaxMinutes = 180", template)
        # 3. The reaction pass reads a queued chat command for a masterless bot (talents / spells answers).
        engine = (ROOT / "ai/playerbot/strategy/ReactionEngine.cpp").read_text()
        self.assertIn("ReactionPassForced(ai->HasRealPlayerMaster(), ai->QueuedChatCommandCount())", engine)
        # 4. `run start ... from ...` resolves the step and guards a start partway.
        start = body(COMMANDS, "static bool HandleDungeonRun(", "\nvoid UpdateRemoteCommanders(")
        self.assertIn("ResolveStartStep(*plan, startArgs.from, startArgs.value, startStep, &movedBack)", start)
        self.assertIn("CheckPartialStart(OnInstanceMap(lead), members.size(), withLead)", start)
        self.assertIn("slot.run = Start(plan, startStep);", start)

    def test_rez_hold_is_wired_behind_its_key(self):
        # AiPlayerbot.DungeonRunRezHold (default 0): the glue reads the rez roster only with the key on and someone
        # dead; the policy asks RezHoldPolicy.h only when the facts carry it, else the old fixed wait runs.
        config = (ROOT / "ai/playerbot/PlayerbotAIConfig.cpp").read_text()
        header = (ROOT / "ai/playerbot/PlayerbotAIConfig.h").read_text()
        template = (ROOT / "ai/playerbot/aiplayerbot.conf.dist.in").read_text()
        self.assertIn('dungeonRunRezHold = config.GetBoolDefault("AiPlayerbot.DungeonRunRezHold", false);', config)
        self.assertIn("bool dungeonRunRezHold;", header)
        self.assertIn("AiPlayerbot.DungeonRunRezHold = 0", template)
        self.assertIn("if (sPlayerbotAIConfig.dungeonRunRezHold && f.dead > 0)\n        ReadRezRoster(lead, bots, f);", COMMANDS)
        roster = body(COMMANDS, "void ReadRezRoster(", "\n}\n")
        self.assertIn("f.rezHold = true;", roster)
        # Rebirth's cooldown is a category cooldown and its seed a reagent; a cast under way holds the budget.
        self.assertIn("bot->HasSpellCategoryCooldown(spell->Category)", roster)
        self.assertIn("spell->Reagent[k] > 0 && !bot->HasItemCount(", roster)
        self.assertIn("f.rezInProgress = true;", roster)
        self.assertIn("bot->IsRessurectRequested()", roster)
        self.assertIn("m.blocksDruid =", roster)
        policy = (ROOT / "runtime/DungeonRunPolicy.h").read_text()
        self.assertIn("if (f.rezHold && !r.rezBodyUnreachable)", policy)
        self.assertIn("else if (f.rezzer && !r.reviveSent && !r.rezBodyUnreachable && r.regroupMs < kPartyRezWaitMs)", policy)
        self.assertIn("TortoiseBots::RezHold::Decide(hold, f.rezMembers)", policy)
        for doc in ("docs/server/settings/dungeons-raids.md",):
            self.assertIn("AiPlayerbot.DungeonRunRezHold", (ROOT / doc).read_text())

    def test_loot_ffa_is_wired_behind_its_key(self):
        # AiPlayerbot.DungeonRunLootFfa (default 0): the glue reads the group's loot method and the lead bot's
        # leadership only with the key on; the loot method goes back whenever the run is not running.
        config = (ROOT / "ai/playerbot/PlayerbotAIConfig.cpp").read_text()
        header = (ROOT / "ai/playerbot/PlayerbotAIConfig.h").read_text()
        template = (ROOT / "ai/playerbot/aiplayerbot.conf.dist.in").read_text()
        self.assertIn('dungeonRunLootFfa = config.GetBoolDefault("AiPlayerbot.DungeonRunLootFfa", false);', config)
        self.assertIn("bool dungeonRunLootFfa;", header)
        self.assertIn("AiPlayerbot.DungeonRunLootFfa = 0", template)
        facts = body(COMMANDS, "void ReadRunFacts(", "\n    f.leadAlive")
        self.assertIn("f.lootFfaOn = sPlayerbotAIConfig.dungeonRunLootFfa;\n    if (f.lootFfaOn || slot.run.lootFfaLive)", facts)
        self.assertIn("f.lootMethod = static_cast<int>(leadGroup->GetLootMethod());", facts)
        self.assertIn("f.leadLeadsGroup = leadGroup->IsLeader(lead->GetObjectGuid());", facts)
        restore = body(COMMANDS, "void RestoreRunLootMethod(", "\n}\n")
        self.assertIn("run.lootFfaLive = false;", restore)
        self.assertIn("group->IsLeader(lead->GetObjectGuid())", restore)
        self.assertIn("static_cast<int>(group->GetLootMethod()) != RemoteCommander::kLootFfa", restore)
        self.assertIn("HandleLootMethodOpcode(packet)", restore)
        # Every way a run stops being a running one restores: pause / stop / time limit (ReleaseRunHold),
        # an unbind (ReleaseBinding), the key switched off.
        hold = body(COMMANDS, "void ReleaseRunHold(", "\n    if (slot.heldRaw.empty()")
        self.assertIn("RestoreRunLootMethod(leadRaw, slot.run);", hold)
        release = body(COMMANDS, "uint32 ReleaseBinding(", "RemoteCommander::Binding removed =")
        self.assertIn("RestoreRunLootMethod(standing->leadGuid, run->second.run);", release)
        off = body(COMMANDS, "// Switched off by a config reload", "s_dungeonRunMs = 0;")
        self.assertIn("RestoreRunLootMethod(standing->leadGuid, entry.second.run);", off)
        policy = (ROOT / "runtime/DungeonRunPolicy.h").read_text()
        self.assertIn("LootFfaWanted(r, f, s)", policy)
        self.assertIn('order = "lootmethod ffa";', policy)
        self.assertIn('std::string("lootmethod ") + RemoteCommander::LootMethodName(r.lootRestore)', policy)
        self.assertIn("AiPlayerbot.DungeonRunLootFfa", (ROOT / "docs/server/settings/dungeons-raids.md").read_text())

    def test_uldaman_run4_fixes_are_wired(self):
        # The tank's reach for a pull stops once a mob has aggro: the facts are read in fights, the policy asks, the
        # glue ends the lead bot's pull.
        guard = body(COMMANDS, "void ReadGuardFacts(", "\n}\n")
        self.assertIn("ReadReachFacts(lead, leadAi, bots, f);", guard)
        reach = body(COMMANDS, "void ReadReachFacts(", "\n}\n")
        self.assertIn("f.reachAggro", reach)
        self.assertIn("f.leadReaching", reach)
        self.assertIn("pull->IsCommandActive()", reach)
        stop = body(COMMANDS, "void StopLeadReach(", "\n}\n")
        self.assertIn('"pull end"', stop)
        tick = body(COMMANDS, "void TickDungeonRun(", "\n}\n")
        self.assertIn("if (it->second.run.stopReachDue)", tick)
        self.assertIn("StopLeadReach(commander, lead);", tick)
        # The healer: held on its stand in the run leader's pulls, behind the tank, kept to heals for the run.
        self.assertIn("context.runLeader = true;", COMMANDS)
        self.assertIn("HealerKeptToHeals(running, true, bot == lead, PlayerbotAI::IsHeal(bot))", COMMANDS)
        ctx = (ROOT / "commands/BotCommandContext.cpp").read_text()
        pause = body(ctx, "void PausePartyDpsForPull(", "\n}\n")
        self.assertIn("(healer && !context.runLeader)", pause)
        self.assertIn("context.noteHealOnly(bot);", pause)
        sync = body(COMMANDS, "void SyncRunHealOnly()\n{", "\n}\n")
        self.assertIn("CommandedBots(group, lead)", sync)
        self.assertIn("HealerKeptToHeals(running, true, bot == lead, PlayerbotAI::IsHeal(bot))", sync)
        self.assertNotIn("healerAi->SetRunHealOnly(true);", tick)
        self.assertIn("SyncRunBackPassive();\n    SyncRunHealOnly();", COMMANDS)
        self.assertIn("ai->SetRunHealOnly(true);", pause)
        self.assertIn("context.healerStand(bot, hx, hy, hz)", pause)
        trig = (ROOT / "ai/playerbot/strategy/triggers/GenericTriggers.cpp").read_text()
        self.assertIn("gate.healOnly = ai->RunHealOnly();", trig)
        self.assertIn("if (ai->RunHealOnly())\n        return false;", trig)
        combat = (ROOT / "ai/playerbot/strategy/generic/CombatStrategy.cpp").read_text()
        self.assertIn("if (ai->RunHealOnly() && PlayerbotAI::IsHeal(ai->GetBot()))\n        return false;", combat)
        # A refused direct step names the dead bot, else the farthest, not the first one checked.
        direct = body(COMMANDS, "static bool CommanderMoveDirect(ChatHandler* handler, CommanderOrder const& order,\n"
                      "    RemoteCommander::MovePoint const& point, std::string const& intent)\n{", "\n}\n")
        self.assertIn("refusedDead", direct)
        self.assertIn("horizontal > refusedYd", direct)


    def test_pull_step_patrollers_reach_the_policy(self):
        # DungeonRun::WaitForPatroller (Uldaman r10 step 66): a pull step's living entries on a waypoint patrol outside
        # the radius, within kPatrolSeeYd beyond it, none a later step owns; the grid search of a pull step reaches that far.
        read = body(COMMANDS, "void ReadRunFacts(DungeonRunSlot const& slot,", "\n}\n")
        add = read[read.index("auto addPatroller"):read.index("f.patrollers.push_back(unit);")]
        self.assertIn("bool const pullStep = step->kind == ai::PlanStepKind::Pull;", read)
        self.assertIn("creature->GetDefaultMovementType() != WAYPOINT_MOTION_TYPE", add)
        self.assertIn("fromPoint <= step->radius || fromPoint > step->radius + kPatrolSeeYd", add)
        self.assertIn("ai::OwnedByLaterStep(*slot.run.plan, slot.run.step, creature->GetEntry(),", add)
        self.assertIn("pullStep ? std::min(toPoint + step->radius + kPatrolSeeYd, 2.0f * kSeeYd) : range", read)
        self.assertIn("unit.z = creature->GetPositionZ();", add)
        self.assertIn("unit.distSpot = fromPoint;", add)
        # Only living ones: the corpse branch lists none (the policy never sees a dead patroller).
        self.assertEqual(read.count("addPatroller(creature, fromPoint)"), 1)
        alive = read.index("if (!creature->IsAlive())")
        self.assertLess(alive, read.index("addPatroller(creature, fromPoint)"))
        self.assertNotIn("addPatroller", read[alive:read.index("if (fromPoint > step->radius)", alive)])
        # The wait and the pull are the policy's: the glue never orders a patroller.
        self.assertNotIn("pullback", add)

    def test_aggro_estimate_is_a_read_side_fact_behind_its_key(self):
        # AiPlayerbot.DungeonRunAggroEstimate (default 0): the glue reads the estimate, the policy decides.
        read = body(COMMANDS, "void ReadRunFacts(DungeonRunSlot const& slot,", "\n}\n")
        self.assertIn("f.joinerCeiling = sPlayerbotAIConfig.dungeonRunAggroEstimate ? "
                      "sPlayerbotAIConfig.dungeonRunAggroCeiling : 0;", read)
        self.assertIn("if (step->kind == ai::PlanStepKind::Pull)\n"
                      "                ReadPullJoiners(lead, bots, *step, toPoint, f.joinerCeiling, slot.run.joinerHeld, f);", read)
        joiners = body(COMMANDS, "void ReadPullJoiners(", "\n}\n")
        self.assertIn("if (!ceiling || f.units.empty())\n        return;", joiners)
        self.assertIn("ai::EstimateAggro(mobs, it->second, params)", joiners)
        self.assertIn("ai::JoinersOverCeiling(estimate, ceiling);", joiners)
        self.assertIn("sCreatureLinkingMgr.GetLinkedTriggerInformation(", joiners)
        self.assertIn("group->HasGroupFlag(OPTION_AGGRO_TOGETHER)", joiners)
        self.assertIn("creatures[i]->IsWithinLOS(params.engageX, params.engageY, params.engageZ + 1.0f, true, 0.0f)", joiners)
        self.assertIn("held.count(unit.guid)", joiners)
        self.assertIn("&& !toMaster", joiners)
        config = (ROOT / "ai/playerbot/PlayerbotAIConfig.cpp").read_text()
        template = (ROOT / "ai/playerbot/aiplayerbot.conf.dist.in").read_text()
        self.assertIn('"AiPlayerbot.DungeonRunAggroEstimate", false', config)
        self.assertIn('"AiPlayerbot.DungeonRunAggroCeiling", 6', config)
        self.assertIn("AiPlayerbot.DungeonRunAggroEstimate = 0", template)
        self.assertIn("AiPlayerbot.DungeonRunAggroCeiling = 6", template)
        header = (ROOT / "ai/playerbot/PullAggroEstimatePolicy.h").read_text()
        self.assertIn("jrad7/mod-dungeon-clear", header)
        self.assertIn("@ da0bdfbc (AGPL-3.0-or-later;", header)
        self.assertIn("Copyright (C) 2026+ Jared Wright and", header)

    def test_pull_stand_is_wired_behind_its_key(self):
        # AiPlayerbot.DungeonRunPullStand (default 0): the glue reads sight, the shot range and the stand only with
        # the key on; the policy holds the pullback for them only when the facts say the key is on.
        read = body(COMMANDS, "void ReadRunFacts(DungeonRunSlot const& slot,", "\n}\n")
        self.assertIn("f.pullStand = sPlayerbotAIConfig.dungeonRunPullStand;", read)
        # The step units (pull/boss) and the hop units carry losLead, from the lead bot's own sight test.
        self.assertIn("if (f.pullStand)\n                        unit.losLead = lead->IsWithinLOSInMap(creature);\n"
                      "                    f.units.push_back(unit);", read)
        self.assertIn("if (f.pullStand)\n                    fact.losLead = lead->IsWithinLOSInMap(unit);\n"
                      "                f.hopUnits.push_back(fact);", read)
        self.assertTrue(read.rstrip().endswith("ReadPullStand(slot, lead, f);"))
        stand = body(COMMANDS, "void ReadPullStand(DungeonRunSlot const& slot,", "\n}\n")
        self.assertIn("if (!f.pullStand || !lead->IsAlive() || !lead->IsInWorld())\n        return;", stand)
        # leadShotYd is the module's PullStrategy::GetRange(), for a ranged weapon shot only (0 for spell and body pulls).
        self.assertIn('if (pull && pull->GetPullActionName() == "shoot")\n        f.leadShotYd = pull->GetRange();',
                      stand)
        self.assertNotIn("f.leadShotYd = ", stand.replace("f.leadShotYd = pull->GetRange();", ""))
        # The stand: only for Run::shotStandFor, and the path search comes after that check.
        guard = stand.index("uint64 const guid = slot.run.shotStandFor;\n    if (!guid || f.leadShotYd <= kShotMarginYd)\n"
                            "        return;")
        self.assertGreater(stand.index("PathFinder path(lead);"), guard)
        self.assertIn("map->GetCreature(ObjectGuid(guid))", stand)
        self.assertIn("map->isInLineOfSight(point.x, point.y, point.z + leadHeight, goal.x, goal.y, goal.z + unitHeight, true)",
                      stand)
        self.assertIn("ShotStandOnPath(points, goal, f.leadShotYd - kShotMarginYd, sees);", stand)
        self.assertIn("f.shotStandGuid = guid;", stand)
        # The policy: PullUnit's pullback passes through the gate first; HoldForShot is off without the key or a range.
        policy = (ROOT / "runtime/DungeonRunPolicy.h").read_text()
        pull = body(policy, "inline void PullUnit(", "\n}\n")
        gate = pull.index("HoldForShot(r, f, u, dt, order))\n            return;")
        self.assertLess(pull.index("if (f.pullStand)\n"), gate)
        self.assertLess(gate, pull.index('order = "pullback " + std::to_string(u.guid);'))
        self.assertLess(gate, pull.index("++tries;"))
        on = body(policy, "inline bool PullStandOn(", "\n}\n")
        self.assertIn("return f.pullStand && f.leadShotYd > kShotMarginYd && !InEvent(r);", on)
        # The hold ends with any tick the gate did not run on (no stale stand request for the glue).
        tick = body(policy, "inline std::string Tick(", "\n}\n")
        self.assertIn("r.shotGateRan = false;", tick)
        self.assertIn("if (!r.shotGateRan)\n        EndShotHold(r);", tick)
        hold = body(policy, "inline bool HoldForShot(", "\n}\n")
        self.assertIn("if (!PullStandOn(r, f))", hold)
        self.assertIn("r.shotStandFor = u.guid;", hold)
        # Every pull caller hands its tick length to the stand walk.
        self.assertIn("PullUnit(r, f, *pick, order, PickCc(r, f, pick->guid), false, dt);", policy)
        self.assertIn("PullUnit(r, f, *hopUnit, order, nullptr, false, dt, true);", policy)
        self.assertIn("PullUnit(r, f, *hopUnit, order, nullptr, true, dt);", policy)
        # PLAN-ZF-NOFIGHT: a move's and the walk back's pulls are hop pulls; a hop unit the tank cannot shoot is
        # left out with no order (after the stand gate, before any pullback). The run-wide set holds hop units
        # only, is read only by the two hop picks, is cleared by every regroup, and loses a unit that attacks.
        self.assertIn("hop = hop || walkBack;", pull)
        nohop = pull.index("if (hop && PullStandOn(r, f) && !ShootableFromLead(f, u))")
        self.assertLess(gate, nohop)
        self.assertLess(nohop, pull.index('order = "pullback " + std::to_string(u.guid);'))
        self.assertIn("r.pullHop = hop;", pull)
        reset = body(policy, "inline void ResetStep(", "\n}\n")
        self.assertIn("r.passed.clear();", reset)
        self.assertNotIn("neverCame", reset)
        await_pull = body(policy, "inline void AwaitPull(", "\n}\n")
        self.assertIn("if (r.pullHop)\n        r.neverCame.insert(r.pullGuid);", await_pull)
        self.assertEqual(policy.count("PickPull(r, f.hopUnits, 0, true)"), 2)
        self.assertEqual(policy.count("r.neverCame.count("), 1)
        regroup = body(policy, "inline void BeginRegroup(", "\n}\n")
        self.assertIn("r.neverCame.clear();", regroup)
        tick_body = body(policy, "inline std::string TickBody(Run& r, const Facts& f, std::uint32_t dt, const RestLines& lines)\n{",
                         "\n}\n")
        self.assertIn("ForgetUnitsThatCame(r, f);", tick_body)
        config = (ROOT / "ai/playerbot/PlayerbotAIConfig.cpp").read_text()
        header = (ROOT / "ai/playerbot/PlayerbotAIConfig.h").read_text()
        template = (ROOT / "ai/playerbot/aiplayerbot.conf.dist.in").read_text()
        self.assertIn('dungeonRunPullStand = config.GetBoolDefault("AiPlayerbot.DungeonRunPullStand", false);', config)
        self.assertIn("bool dungeonRunPullStand;", header)
        self.assertIn("AiPlayerbot.DungeonRunPullStand = 0", template)
        for doc in ("docs/server/settings/dungeons-raids.md", "docs/play/dungeon-run-leader.md"):
            self.assertIn("AiPlayerbot.DungeonRunPullStand", (ROOT / doc).read_text())


    def test_hold_follows_the_tank_and_dead_gossip_npc_is_named(self):
        policy = (ROOT / "runtime/DungeonRunPolicy.h").read_text()
        # 1. A hold's fight lets the places go when the tank is off the spot; the glue's existing release
        #    (SyncHoldStands -> FormUpOnLead) is what makes the placed bots follow it.
        fight = body(policy, "        ReleaseHoldStands(r, f);\n", "        if (CheckGuard(r, f, dt, order))")
        self.assertIn("CheckReach(r, f);", fight)
        release = body(policy, "inline bool ReleaseHoldStands(", "\n}\n")
        self.assertIn("++r.holdStandsGen;", release)
        self.assertIn("r.holdPlaced = false;", release)
        stands = body(COMMANDS, "void SyncHoldStands(", "\n}\n")
        self.assertIn("else if (!stand && placed)", stands)
        self.assertIn("FormUpOnLead(commander, lead, released, false);", stands)
        self.assertIn("if (bot->IsAlive() && run.state == TortoiseBots::DungeonRun::State::Running)", stands)
        # 2. The module names a dead gossip NPC in the no-npc text, and the policy reads that wording.
        gossip = body(COMMANDS, "static bool CommanderGossip(", "\n}\n")
        no_npc = body(gossip, "if (!target.creature)", "return true;")
        self.assertIn("DeadGossipNpcText(bot, request.entry)", no_npc)
        self.assertIn("DeadGossipNpcText(bot, task.gossip.entry)", COMMANDS)
        text = body(COMMANDS, "std::string DeadGossipNpcText(", "\n}\n")
        self.assertIn("!creature->IsAlive()", body(COMMANDS, "class EntryDeadCreatureCheck", "};"))
        self.assertIn(") lies dead ", text)
        self.assertIn('constexpr const char* kGossipNpcDeadMark = "lies dead";', policy)
        refused = body(policy, "inline void OrderRefused(", "\n}\n")
        self.assertIn("GossipNpcDeadNote(", refused)
        gossip_step = body(policy, "inline void GossipStep(", "\n}\n")
        self.assertIn("GossipNpcDeadNote(what, f.taskText)", gossip_step)


if __name__ == "__main__":
    unittest.main()
