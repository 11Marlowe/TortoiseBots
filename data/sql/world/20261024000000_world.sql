-- Zul'Farrak (map 209): the move that walks the party into area trigger 962 (at_zumrah: Witch Doctor Zum'rah turns
-- hostile; leg 10 seq 101) names that trigger in `entry`. A move or move_direct step's `entry` is the area trigger the
-- step walks into; it was unused for these kinds. While the run stands at that step its bots fire the trigger's script
-- although AiPlayerbot.BotAreaTriggers does not list it (plan-driven, like the gossip steps' NPCs), so a shipped plan
-- needs no allowlist edit; every other unlisted trigger stays off. Idempotent. To be mirrored in turtle-agent
-- dungeons/zulfarrak.json (exported into `entry`): it sorts after the whole-plan export 20261022210000, and a re-export
-- made before that turtle-agent change carries entry 0 again, so this UPDATE must then follow it.
UPDATE `ai_dungeon_step` SET `entry` = 962
WHERE `plan` = 'zulfarrak' AND `leg` = 10 AND `seq` = 101 AND `kind` = 'move';
