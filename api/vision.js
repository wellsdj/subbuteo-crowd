const VISION_PROMPT = `You are watching a Subbuteo tabletop football game. Subbuteo is played on a flat green felt surface with small plastic player figures (about 2–3 cm tall) and a small round ball (about 1 cm).

Analyse this image and respond ONLY with a valid JSON object — no other text, no markdown, no explanation.

Required JSON:
{
  "ballPosition": <one of: "in-left-goal", "near-left-goal", "midfield", "near-right-goal", "in-right-goal", "not-visible">,
  "keeperBlocking": <true if a goalkeeper figure is clearly between the ball and the nearest goal, otherwise false>,
  "confidence": <"high", "medium", or "low">
}

Definitions:
- "in-left-goal": ball is visibly inside the left-hand goal structure or net
- "near-left-goal": ball is close to the left goal (within roughly the penalty area)
- "midfield": ball is in the central portion of the pitch
- "near-right-goal": ball is close to the right goal (within roughly the penalty area)
- "in-right-goal": ball is visibly inside the right-hand goal structure or net
- "not-visible": you cannot locate the ball with confidence

Use "low" confidence if lighting is poor, the view is partially obscured, or you are guessing.`;

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
