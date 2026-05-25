const VISION_PROMPT = `You are analysing a camera shot of a Subbuteo tabletop football game to determine where the ball is.

WHAT YOU ARE LOOKING AT:
- A rectangular green felt pitch with white painted lines
- Small plastic player figures (~2-3 cm) on circular bases — these are NOT the ball
- One small round ball (~1 cm diameter), usually white or off-white
- Two small rectangular goal frames/nets at each SHORT end of the pitch
- White lines painted on the felt: a centre circle, and near each goal a penalty area (a rectangular box or D-shape)

STEP 1 — FIND THE PITCH LINES.
Even if the camera is at an angle, identify:
  - The centre line or centre circle (divides the pitch in half)
  - The penalty area line near each goal (the rectangular box closest to the goal)
The penalty area line is your key boundary — it separates "near-goal" from "midfield".

STEP 2 — FIND THE BALL.
The ball is the smallest round object. It is NOT a player base (which are larger flat circles).
Scan carefully near figures and near the goals. Account for perspective — the ball may look smaller toward the far end.

STEP 3 — CLASSIFY using the pitch lines as reference, NOT by estimating percentages:

  "in-goal"     — ball is inside or past the goal frame/net. HIGHEST PRIORITY. If ball is touching or beyond the goal posts, use this.
  "inline-goal" — ball is between the penalty area line and the goal, on the central axis directly facing the goal mouth
  "near-goal"   — ball is between the penalty area line and the goal, but at a wide angle OR ball is near the penalty area line on either end
  "midfield"    — ball is clearly in the central zone, outside both penalty areas (between the two penalty area lines)
  "not-visible" — cannot locate the ball after careful inspection

CAMERA ANGLE GUIDANCE:
If the camera is not directly overhead, use the painted lines to judge depth:
- A ball that appears to be "near" a goal in the image but is actually behind the penalty area line = midfield
- Use the lines as your reference, not just the ball's position in the image frame
- If the penalty area line is visible, anything on the goal side of it = near-goal or in-goal

CONFIDENCE:
- "high"   — ball clearly visible, zone certain using the pitch lines
- "medium" — ball likely visible, zone judged from pitch lines
- "low"    — lines not visible, ball hidden, or genuinely unsure

KEEPER BLOCKING: true only if a goalkeeper figure is clearly between the ball and the nearest goal.

BIAS RULES:
- When unsure between "near-goal" and "midfield", prefer "near-goal"
- When unsure between "in-goal" and "near-goal", prefer "in-goal"
- Never use "midfield" if the ball is visibly inside a penalty area

Respond ONLY with valid JSON, no other text:
{
  "ballPosition": "in-goal" | "inline-goal" | "near-goal" | "midfield" | "not-visible",
  "keeperBlocking": true | false,
  "confidence": "high" | "medium" | "low"
}`;

module.exports = async function handler(req, res) {
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
};
