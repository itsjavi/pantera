# Development backlog

Pantera's plan, tasks and decisions, kept as plain Markdown in the repository so people and
agents work from the same list. It's a lightweight, portable take on
[Backlog.md](https://github.com/MrLesk/Backlog.md): no tool to install, just files and one Python
script. Because it lives in git, it is also the agents' durable memory: it survives new sessions,
clones and machines.

```
devbacklog/
├── roadmap.md          High-level goals, one section per phase
├── decisions/          Decision records: what was decided and why
├── scripts/backlog.py  The tool to read and change the backlog
└── tasks/
    ├── draft/          Ideas and work that isn't ready to start
    ├── todo/           Ready to start
    ├── in-progress/    Being worked on
    └── done/           Finished and verified
```

## Tasks

Each task is one file named `TASK-N-short-title.md`. The folder it sits in is its state; there is
no status field to keep in sync.

```markdown
---
id: TASK-3
title: Restore Chrome Web Store installs
priority: high
labels: [phase-0, extensions]
depends_on: [TASK-4]
created: 2026-10-27
---

## Goal

What should be true when this is done, and why.

## Acceptance criteria

- [ ] Checkable outcomes, ticked as they're met

## Notes

Findings, decisions, links, open questions.
```

| Field | Rules |
|---|---|
| `id` | `TASK-N`. Numbers increase and are never reused or renumbered. |
| `title` | One line, imperative ("Restore…", "Add…"). |
| `priority` | `high`, `medium` or `low`. |
| `labels` | Optional, `[a, b]`, lowercase-kebab-case. Every task gets its phase (`phase-0` … `phase-3`, see [roadmap.md](roadmap.md)) plus area labels such as `extensions`, `privacy`, `build`, `l10n`. |
| `depends_on` | Optional, `[TASK-1, TASK-2]`. A task isn't ready until these are done. |
| `created` | `YYYY-MM-DD`. |
| `completed` | Added by the script when a task moves to `done`. |

The frontmatter is a strict subset of YAML: one `key: value` per line, lists in `[a, b]` form,
no comments or nested values.

## Decisions

Each decision is one file in `decisions/`, named `DECISION-N-short-title.md`. Record a decision
whenever a choice constrains future work: product identity, privacy trade-offs, architecture,
anything someone might otherwise undo by accident.

```markdown
---
id: DECISION-4
title: Web Store extensions update only when the user asks
date: 2026-10-27
status: accepted
tasks: [TASK-3]
---

## Context

The problem and the forces at play.

## Decision

What was decided.

## Consequences

What follows from it, including the downsides.
```

| Field | Rules |
|---|---|
| `id` | `DECISION-N`, never reused or renumbered. |
| `status` | `proposed`, `accepted`, `superseded` or `rejected`. |
| `tasks` | Optional, `[TASK-1]`: the tasks it came from or affects. |
| `superseded_by` | Required when `status: superseded`: the `DECISION-N` that replaces it. |

Don't edit an accepted decision to change its meaning. Record a new one, then mark the old one
`superseded` with `superseded_by`.

## States

- **draft**: an idea or a task still missing a clear goal or acceptance criteria.
- **todo**: ready to start. It has a goal and acceptance criteria.
- **in-progress**: someone is working on it. Keep it to one task per person or agent.
- **done**: every acceptance criterion is met and verified, and the work is committed.

## The script

Run it from the repository root. It needs only Python 3.

```bash
devbacklog/scripts/backlog.py status                 # counts, in progress, next up, waiting
devbacklog/scripts/backlog.py next                   # the task to pick up next
devbacklog/scripts/backlog.py list todo --label privacy
devbacklog/scripts/backlog.py show TASK-3
devbacklog/scripts/backlog.py new "Add Sparkle updates" --priority medium --labels phase-1,updates --state draft
devbacklog/scripts/backlog.py move TASK-3 in-progress
devbacklog/scripts/backlog.py decisions --task TASK-3
devbacklog/scripts/backlog.py show DECISION-4
devbacklog/scripts/backlog.py decide "Use Sparkle for updates" --tasks TASK-6
devbacklog/scripts/backlog.py check                  # validate every task and decision
```

`show TASK-N` also lists the decisions linked to that task.

**Next up** is the highest-priority task in `todo` whose dependencies are all done; ties go to
the lowest number.

## Workflow

1. Check `status` before starting work.
2. Take `next`, unless someone asked for something specific. Move the task to `in-progress`.
   Read its linked decisions, and any others for the area you're changing.
3. Do the work. Tick acceptance criteria as they're met and add findings to Notes. When a choice
   is made along the way, record it with `decide`.
4. When everything is verified, move the task to `done` and commit the move with the change.
5. Put follow-up work you discover in a new task (`draft` if it's unclear, `todo` if it's ready)
   instead of growing the current one.
6. Run `check` before committing backlog changes.
