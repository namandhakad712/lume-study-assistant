# AI Agent ("Lume" Brain)

The device's intelligence is a **Tuya AI Agent** — the "Brain" — configured on the
Tuya Developer Platform and bound to this product. It is a **cloud voice agent**:
the device streams mic audio up, the agent answers, TTS comes back down.

## Agent identity

| | |
|---|---|
| Agent project | Study Assist Brain |
| Project code | `aipt_fvjuqr11yk8w` |
| Bound product | `okqfzw6tkrabylcs` (Rankify Assist) |
| Where to edit | TuyaOpen IDE → **AI Agent → Brain → Study Assist Brain** → prompt |
| Publish | save prompt → **publish** |

<div align="center" style="border:2px dashed #999; border-radius:12px; padding:24px; margin:24px 0; background:#fafafa;">
  📸 <strong>Screenshot placeholder</strong> — Study Brain settings page in the IDE<br/>
  <small>Save as <code>docs/images/brain-settings.png</code> and add: <code>&lt;img src="images/brain-settings.png"&gt;</code></small>
</div>

## System prompt

The full prompt lives in [`docs/agent-prompt.md`](agent-prompt.md) (paste-ready).
Highlights:

- **Voice-first**: answers < 15 s of speech, one idea per sentence, no lists/tables.
- **Language**: matches the student (Hinglish / Hindi / English).
- **Exam-prep focus**: syllabus-first (NCERT-first for board/entrance), step-by-step
  problems, PYQ guidance, depth matched to the exam (board / NEET / JEE / school).
- **Memory**: remembers name, subject, study goal; asks when unsure.
- **Focus timer — strict contract** (the only timer the agent handles):

  | Event | Agent behavior |
  |-------|----------------|
  | Timer started | device tells the agent (`"…started the study focus timer for <N> minutes. Remember this."`) |
  | Timer ended | device tells the agent → ask "Did you finish your study goal?" → celebrate / re-plan |
  | User asks "set a timer" | **"I can't set timers. Please use the focus timer in the app."** |
  | User asks time left | **"I can't see the timer. Check the app."** |
  | Anything else timer-ish | never creates cloud timers, never claims to track time |

- **Honesty**: never claims to see device state (mute/LED/volume/mode) unless the
  device told it.

## Device → agent injection (firmware)

`source/embedded/src/app_dp_ctrl.c` — `__app_dp_agent_text()` wraps
`tuya_ai_text_input` with a readiness check and retry loop. Trigger points:

| Trigger | Injected text |
|---------|---------------|
| Focus timer start | `The user started the study focus timer for <N> minutes. Remember this.` |
| Focus timer end | `The focus timer has ended. Ask the user whether they completed their study goal.` |

The agent only knows what the device tells it — it has no device-state visibility.

## Why no cloud timers

The agent once created a **cloud timer** via the product's BIC `timer` function
when asked for a "5 minute timer". The timer fired on time (cloud-side) but
delivered its action as **DP 207** — which does not exist in the product schema
(`DP ID 207 Invalid`) — so the device never reacted.

Conclusion: cloud timers are unreliable for this product. The strict prompt above
is the fix; the firmware never creates cloud timers itself.

## Editing & publishing

1. Open TuyaOpen IDE → Developer Platform → **AI Agent**.
2. Select **Study Assist Brain** (`aipt_fvjuqr11yk8w`).
3. Paste the prompt from [`agent-prompt.md`](agent-prompt.md).
4. **Save**, then **Publish** (unpublished changes are not served to devices).
5. Changes apply to new chat sessions; a device restart is not required.

> The CLI has no prompt-write command — the prompt must be pasted in the IDE.