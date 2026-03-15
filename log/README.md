# Task Log Guide

Use this folder to record agent task completion notes.

## When to Log
- Create one log entry after each discrete completed task.
- If a task is split into sub-tasks, log each sub-task when it is finished.

## File Naming
- Preferred: `YYYY-MM-DD.md` (append multiple entries by time).
- Alternative: `YYYY-MM-DD_HHMM_task-slug.md` for isolated task files.

## Entry Template
```md
## YYYY-MM-DD HH:MM

- Task: <short summary>
- Files changed: <path1>, <path2>, ...
- Validation: <build/test/lint commands and outcomes>
- Notes: <assumptions, risks, follow-up>
```

## Example
```md
## 2026-03-15 19:10

- Task: Add AGENTS.md and unify agent instruction entry point
- Files changed: AGENTS.md, AGENT.md
- Validation: Manual content review; no build system configured yet
- Notes: AGENT.md now points to AGENTS.md as authoritative
```

Keep entries concise and factual so future agents can understand progress quickly.
