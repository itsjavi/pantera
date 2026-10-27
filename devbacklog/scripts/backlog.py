#!/usr/bin/env python3
"""Pantera's development backlog: one Markdown file per task, one folder per state.

Usage:
  backlog.py status                     Counts per state, tasks in progress and what's next.
  backlog.py next                       The todo task to pick up next.
  backlog.py list [STATE ...] [--label LABEL] [--priority PRIORITY]
  backlog.py show TASK-N                Print a task.
  backlog.py new TITLE [--priority P] [--labels a,b] [--depends TASK-1,TASK-2] [--state S]
  backlog.py move TASK-N STATE          Move a task to another state.
  backlog.py check                      Validate every task file.

See devbacklog/README.md for the task format and workflow.
"""

import argparse
import datetime
import re
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TASKS = ROOT / "tasks"
STATES = ("draft", "todo", "in-progress", "done")
PRIORITIES = ("high", "medium", "low")
REQUIRED = ("id", "title", "priority", "created")
ID = re.compile(r"^TASK-(\d+)$")
LABEL = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*$")


class BacklogError(Exception):
    pass


@dataclass
class Task:
    path: Path
    state: str
    fields: dict
    body: str

    @property
    def id(self):
        return self.fields.get("id", "")

    @property
    def number(self):
        match = ID.match(self.id)
        return int(match.group(1)) if match else 0

    @property
    def title(self):
        return self.fields.get("title", "")

    @property
    def priority(self):
        return self.fields.get("priority", "")

    @property
    def labels(self):
        return self.fields.get("labels", [])

    @property
    def depends_on(self):
        return self.fields.get("depends_on", [])

    def sort_key(self):
        rank = PRIORITIES.index(self.priority) if self.priority in PRIORITIES else len(PRIORITIES)
        return (rank, self.number)

    def row(self, with_state=False):
        state = f"{self.state:<12}" if with_state else ""
        labels = f"  [{', '.join(self.labels)}]" if self.labels else ""
        return f"{self.id:<9}{state}{self.priority:<8}{self.title}{labels}"


def parse_value(value):
    value = value.strip()
    if value.startswith("[") and value.endswith("]"):
        return [item.strip().strip("'\"") for item in value[1:-1].split(",") if item.strip()]
    return value.strip("'\"")


def read_task(path, state):
    text = path.read_text(encoding="utf-8")
    if not text.startswith("---\n"):
        raise BacklogError(f"{path.relative_to(ROOT)}: missing frontmatter")
    end = text.find("\n---\n", 3)
    if end < 0:
        raise BacklogError(f"{path.relative_to(ROOT)}: frontmatter is not closed with ---")
    fields = {}
    for line in text[4:end].splitlines():
        if not line.strip():
            continue
        key, sep, value = line.partition(":")
        if not sep:
            raise BacklogError(f"{path.relative_to(ROOT)}: frontmatter line is not 'key: value': {line}")
        fields[key.strip()] = parse_value(value)
    return Task(path, state, fields, text[end + 5:])


def load():
    tasks = []
    for state in STATES:
        for path in sorted((TASKS / state).glob("*.md")):
            tasks.append(read_task(path, state))
    return tasks


def find(tasks, task_id):
    for task in tasks:
        if task.id == task_id.upper():
            return task
    raise BacklogError(f"no task {task_id}")


def ready(task, by_id):
    """A todo task is ready when everything it depends on is done."""
    return all(by_id.get(dep) and by_id[dep].state == "done" for dep in task.depends_on)


def next_tasks(tasks):
    by_id = {task.id: task for task in tasks}
    todo = sorted((t for t in tasks if t.state == "todo"), key=Task.sort_key)
    return [t for t in todo if ready(t, by_id)], [t for t in todo if not ready(t, by_id)]


def cmd_status(tasks, args):
    counts = " · ".join(f"{state} {sum(t.state == state for t in tasks)}" for state in STATES)
    print(counts)
    in_progress = sorted((t for t in tasks if t.state == "in-progress"), key=Task.sort_key)
    print("\nIn progress:")
    print("\n".join(f"  {t.row()}" for t in in_progress) or "  (none)")
    ready_tasks, waiting = next_tasks(tasks)
    print("\nNext up:")
    print(f"  {ready_tasks[0].row()}" if ready_tasks else "  (nothing ready in todo)")
    if len(ready_tasks) > 1:
        print("\nAlso ready:")
        print("\n".join(f"  {t.row()}" for t in ready_tasks[1:]))
    if waiting:
        print("\nWaiting on dependencies:")
        print("\n".join(f"  {t.row()}  (needs {', '.join(t.depends_on)})" for t in waiting))


def cmd_next(tasks, args):
    ready_tasks, _ = next_tasks(tasks)
    if not ready_tasks:
        print("Nothing ready in todo.")
        return
    task = ready_tasks[0]
    print(task.row())
    print(task.path.relative_to(ROOT.parent))


def cmd_list(tasks, args):
    states = args.states or list(STATES)
    for state in states:
        if state not in STATES:
            raise BacklogError(f"unknown state {state}; use one of {', '.join(STATES)}")
    rows = [t for t in tasks if t.state in states
            and (not args.label or args.label in t.labels)
            and (not args.priority or t.priority == args.priority)]
    rows.sort(key=lambda t: (STATES.index(t.state), t.sort_key()))
    print("\n".join(t.row(with_state=True) for t in rows) or "(no tasks)")


def cmd_show(tasks, args):
    task = find(tasks, args.id)
    print(f"{task.path.relative_to(ROOT.parent)}  ({task.state})\n")
    print(task.path.read_text(encoding="utf-8"), end="")


def slugify(title):
    return re.sub(r"[^a-z0-9]+", "-", title.lower()).strip("-")[:60].rstrip("-")


def split_list(value):
    return [item.strip() for item in value.split(",") if item.strip()] if value else []


def cmd_new(tasks, args):
    if args.state not in STATES:
        raise BacklogError(f"unknown state {args.state}")
    if args.priority not in PRIORITIES:
        raise BacklogError(f"unknown priority {args.priority}; use one of {', '.join(PRIORITIES)}")
    number = max((t.number for t in tasks), default=0) + 1
    labels, depends = split_list(args.labels), [d.upper() for d in split_list(args.depends)]
    path = TASKS / args.state / f"TASK-{number}-{slugify(args.title)}.md"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        "---\n"
        f"id: TASK-{number}\n"
        f"title: {args.title}\n"
        f"priority: {args.priority}\n"
        f"labels: [{', '.join(labels)}]\n"
        f"depends_on: [{', '.join(depends)}]\n"
        f"created: {datetime.date.today().isoformat()}\n"
        "---\n\n"
        "## Goal\n\n\n"
        "## Acceptance criteria\n\n- [ ] \n\n"
        "## Notes\n",
        encoding="utf-8",
    )
    print(path.relative_to(ROOT.parent))


def cmd_move(tasks, args):
    if args.state not in STATES:
        raise BacklogError(f"unknown state {args.state}; use one of {', '.join(STATES)}")
    task = find(tasks, args.id)
    if task.state == args.state:
        print(f"{task.id} is already {args.state}")
        return
    text = task.path.read_text(encoding="utf-8")
    text = re.sub(r"^completed:.*\n", "", text, count=1, flags=re.M)
    if args.state == "done":
        end = text.find("\n---\n", 3)
        text = f"{text[:end]}\ncompleted: {datetime.date.today().isoformat()}{text[end:]}"
    target = TASKS / args.state / task.path.name
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text, encoding="utf-8")
    task.path.unlink()
    print(f"{task.id}: {task.state} -> {args.state}  ({target.relative_to(ROOT.parent)})")


def cmd_check(tasks, args):
    problems = []
    seen = {}
    for task in tasks:
        name = task.path.relative_to(ROOT)
        for field in REQUIRED:
            if not task.fields.get(field):
                problems.append(f"{name}: missing {field}")
        if task.id and not ID.match(task.id):
            problems.append(f"{name}: id must look like TASK-N, got {task.id}")
        if task.id and not task.path.name.startswith(f"{task.id}-"):
            problems.append(f"{name}: file name must start with {task.id}-")
        if task.id in seen:
            problems.append(f"{name}: duplicate id {task.id} (also {seen[task.id]})")
        seen[task.id] = name
        if task.priority and task.priority not in PRIORITIES:
            problems.append(f"{name}: priority must be one of {', '.join(PRIORITIES)}")
        for key in ("labels", "depends_on"):
            if not isinstance(task.fields.get(key, []), list):
                problems.append(f"{name}: {key} must be a list like [a, b]")
        for label in task.labels:
            if not LABEL.match(label):
                problems.append(f"{name}: label '{label}' must be lowercase-kebab-case")
        if task.state == "done" and not task.fields.get("completed"):
            problems.append(f"{name}: done tasks need a completed date")
    ids = {task.id for task in tasks}
    for task in tasks:
        for dep in task.depends_on:
            if dep not in ids:
                problems.append(f"{task.path.relative_to(ROOT)}: depends on unknown {dep}")
    for path in TASKS.iterdir():
        if path.name.startswith("."):
            continue
        if not path.is_dir() or path.name not in STATES:
            problems.append(f"tasks/{path.name}: only the state folders {', '.join(STATES)} belong in tasks/")
    for state in STATES:
        if not (TASKS / state).is_dir():
            problems.append(f"tasks/{state}/ is missing (keep a .gitkeep in empty state folders)")
            continue
        for path in (TASKS / state).iterdir():
            if not path.name.startswith(".") and path.suffix != ".md":
                problems.append(f"{path.relative_to(ROOT)}: only .md task files belong here")
    if problems:
        print("\n".join(problems))
        sys.exit(1)
    print(f"OK: {len(tasks)} tasks")


def main():
    parser = argparse.ArgumentParser(description="Pantera's development backlog.")
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("status", help="counts, in progress and next up")
    commands.add_parser("next", help="the todo task to pick up next")
    list_parser = commands.add_parser("list", help="list tasks")
    list_parser.add_argument("states", nargs="*", metavar="STATE")
    list_parser.add_argument("--label")
    list_parser.add_argument("--priority", choices=PRIORITIES)
    show_parser = commands.add_parser("show", help="print a task")
    show_parser.add_argument("id")
    new_parser = commands.add_parser("new", help="create a task")
    new_parser.add_argument("title")
    new_parser.add_argument("--priority", default="medium")
    new_parser.add_argument("--labels", default="")
    new_parser.add_argument("--depends", default="")
    new_parser.add_argument("--state", default="todo")
    move_parser = commands.add_parser("move", help="move a task to another state")
    move_parser.add_argument("id")
    move_parser.add_argument("state")
    commands.add_parser("check", help="validate every task file")

    args = parser.parse_args()
    handler = globals()[f"cmd_{args.command}"]
    try:
        handler(load(), args)
    except BacklogError as error:
        sys.exit(f"backlog: {error}")


if __name__ == "__main__":
    main()
