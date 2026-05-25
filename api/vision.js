const VISION_PROMPT = `You are analysing a camera shot of a Subbuteo tabletop football game. The camera is positioned above and to the side of the pitch at an angle.

IMPORTANT — IGNORE HUMANS: There will likely be people, hands, or fingers visible in the frame (players flicking figures, reaching over the table). Ignore all human body parts completely. Only analyse the green felt pitch and what is on it.

WHAT IS ON THE PITCH:
- White painted lines: centre circle/line, and a penalty area box near each goal
- Small plastic player figures (~2-3 cm) on circular bases
- One small round ball (~1 cm), usually white or off-white — smaller than the player bases
- Two rectangular goal frames/nets at each SHORT end of the pitch

HOW TO CLASSIFY — use the white painted lines, not pixel position:

  "in-goal"     — ball is inside or past the goal frame. If the ball is touching or beyond the goal posts, use this. HIGHEST PRIORITY.
  "inline-goal" — ball is in the penalty area, on the central axis directly facing the goal mouth (straight-on shot)
  "near-goal"   — ball is inside the penalty area box OR very close to the penalty area line on either end
  "midfield"    — ball is clearly between the two penalty areas, in the central zone
  "not-visible" — cannot locate the ball

CAMERA ANGLE: The pitch will look like a trapezoid due to perspective. The penalty area lines are still visible — use them as your zone boundaries. The far end of the pitch will appear smaller/higher in the frame.

STRICT RULES — these override everything else:
1. If the ball is anywhere inside a penalty area box → "near-goal" minimum (not midfield)
2. If the ball is touching/inside a goal frame → "in-goal" (not near-goal)
3. When unsure between midfield and near-goal → choose near-goal
4. When unsure between near-goal and in-goal → choose in-goal
5. Human hands near the pitch do not affect classification

KEEPER BLOCKING: true only if the goalkeeper figure is clearly between ball and goal.

CONFIDENCE: "high" if ball clearly visible and zone certain. "medium" if fairly sure. "low" if genuinely unsure.

Respond ONLY with valid JSON:
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
