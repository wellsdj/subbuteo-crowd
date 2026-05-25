const VISION_PROMPT = `You are analysing a live overhead or angled shot of a Subbuteo tabletop football game.

WHAT YOU ARE LOOKING AT:
- A rectangular green felt playing surface (the pitch)
- Small plastic player figures, roughly 2–3 cm tall, on circular bases
- One small round ball, roughly 1 cm diameter — usually white, off-white, or light-coloured
- Two small rectangular goal frames, one at each short end of the table (left end and right end)
- The goalkeeper is a single figure lying flat or on a small rod inside each goal mouth

YOUR TASK — find the ball. It is the smallest round object on the pitch, NOT a player base.
Scan the entire image carefully before deciding. The ball may be partially obscured by a figure.

ZONES (based on horizontal position across the pitch):
- "in-left-goal"    — ball is inside or touching the left goal net/frame
- "near-left-goal"  — ball is in the left-side attacking third, within shooting range of the left goal (roughly the leftmost 25% of the pitch)
- "midfield"        — ball is in the central 50% of the pitch
- "near-right-goal" — ball is in the right-side attacking third, within shooting range of the right goal (roughly the rightmost 25% of the pitch)
- "in-right-goal"   — ball is inside or touching the right goal net/frame
- "not-visible"     — you genuinely cannot locate the ball after careful inspection

KEEPER BLOCKING: set true only if a goalkeeper figure is clearly positioned between the ball and the goal mouth it is threatening.

CONFIDENCE:
- "high"   — you can clearly see the ball and are certain of its zone
- "medium" — you can see something that is very likely the ball
- "low"    — poor lighting, ball obscured, or you are uncertain

Respond ONLY with a valid JSON object, no other text:
{
  "ballPosition": "in-left-goal" | "near-left-goal" | "midfield" | "near-right-goal" | "in-right-goal" | "not-visible",
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
