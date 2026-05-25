const VISION_PROMPT = `You are analysing a live camera shot of a Subbuteo tabletop football game.

WHAT YOU ARE LOOKING AT:
- A rectangular green felt pitch
- Small plastic player figures (~2-3 cm tall) on circular bases — these are NOT the ball
- One small round ball, ~1 cm diameter, usually white or off-white
- Two small rectangular goal frames/nets, one at each SHORT end of the table
- A goalkeeper figure inside each goal mouth

STEP 1 — Find the goals. They are the small rectangular net structures at the two short ends of the table.
STEP 2 — Find the ball. It is the smallest round object on the pitch. Scan carefully — it may be near a figure.
STEP 3 — Classify using EXACTLY one of these zones:

  "in-goal"     — ball is INSIDE a goal net or has clearly crossed the goal line. HIGHEST PRIORITY — if the ball is touching or past the goal frame, use this. Do not use "near-goal" when the ball is in the net.
  "inline-goal" — ball is directly in front of a goal mouth on a central straight shooting line (like a penalty). NOT yet in the net.
  "near-goal"   — ball is within roughly the nearest 25% of the pitch to EITHER goal end, but at an angle or wider position
  "midfield"    — ball is in the central 50% of the pitch, clearly away from both goals
  "not-visible" — you cannot locate the ball after careful inspection

KEEPER BLOCKING: true only if a goalkeeper figure is clearly between the ball and the nearest goal.

CONFIDENCE:
- "high"   — ball clearly visible, zone certain
- "medium" — ball likely visible, zone fairly certain
- "low"    — poor lighting, ball hidden, or genuinely unsure

CRITICAL RULES:
1. If the ball appears to be inside or past a goal frame — always use "in-goal", never downgrade to "near-goal"
2. The attacking thirds (near-goal) cover roughly the area between the penalty spot and the goal — if the ball is anywhere in that zone, use "near-goal" not "midfield"
3. When in doubt between "midfield" and "near-goal", prefer "near-goal"

Respond ONLY with valid JSON, no other text:
{
  "ballPosition": "in-goal" | "inline-goal" | "near-goal" | "midfield" | "not-visible",
  "keeperBlocking": true | false,
  "confidence": "high" | "medium" | "low"
}`;

export default async function handler(req, res) {
  if (req.method !== 'POST') return res.status(405).end();

  const { image } = req.body || {};
  if (!image) return res.status(400).json({ error: 'No image provided' });

  const apiKey = process.env.Anthropic_API_Key;
  if (!apiKey) return res.status(500).json({ error: 'Anthropic_API_Key env var not set' });

  const upstream = await fetch('https://api.anthropic.com/v1/messages', {
    method: 'POST',
    headers: {
      'x-api-key': apiKey,
      'anthropic-version': '2023-06-01',
      'content-type': 'application/json',
    },
    body: JSON.stringify({
      model: 'claude-haiku-4-5-20251001',
      max_tokens: 120,
      messages: [{
        role: 'user',
        content: [
          { type: 'image', source: { type: 'base64', media_type: 'image/jpeg', data: image } },
          { type: 'text',  text: VISION_PROMPT }
        ]
      }]
    })
  });

  const data = await upstream.json();
  res.status(upstream.status).json(data);
}
