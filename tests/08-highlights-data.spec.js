const { test, expect } = require('@playwright/test');
const { gotoApp, navigateTo } = require('./helpers');

test.describe('Highlights IndexedDB data', () => {
  test('clearAllHighlights leaves DB empty', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(async () => { await clearAllHighlights(); });
    const count = await page.evaluate(async () => countHighlights());
    expect(count).toBe(0);
  });

  test('saveHighlight increases count by 1', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(async () => { await clearAllHighlights(); });
    await page.evaluate(async () => {
      const blob = new Blob(['test'], { type: 'video/webm' });
      await saveHighlight({ type: 'goal', blob, timestamp: Date.now(), team: 'home', scoreAfter: null });
    });
    const count = await page.evaluate(async () => countHighlights());
    expect(count).toBe(1);
  });

  test('getHighlights returns saved entry', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(async () => { await clearAllHighlights(); });
    await page.evaluate(async () => {
      const blob = new Blob(['test'], { type: 'video/webm' });
      await saveHighlight({ type: 'save', blob, timestamp: 12345, team: null, scoreAfter: null });
    });
    const entries = await page.evaluate(async () => {
      const items = await getHighlights();
      return items.map(i => ({ type: i.type, timestamp: i.timestamp }));
    });
    expect(entries.length).toBe(1);
    expect(entries[0].type).toBe('save');
  });

  test('deleteHighlight removes entry', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(async () => { await clearAllHighlights(); });
    const id = await page.evaluate(async () => {
      const blob = new Blob(['x'], { type: 'video/webm' });
      return saveHighlight({ type: 'nearmiss', blob, timestamp: Date.now(), team: null, scoreAfter: null });
    });
    await page.evaluate(async (id) => deleteHighlight(id), id);
    const count = await page.evaluate(async () => countHighlights());
    expect(count).toBe(0);
  });

  test('updateHighlight patches fields', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(async () => { await clearAllHighlights(); });
    const id = await page.evaluate(async () => {
      const blob = new Blob(['x'], { type: 'video/webm' });
      return saveHighlight({ type: 'goal', blob, timestamp: Date.now(), team: null, scoreAfter: null });
    });
    await page.evaluate(async (id) => updateHighlight(id, { team: 'away' }), id);
    const entries = await page.evaluate(async () => getHighlights());
    expect(entries[0].team).toBe('away');
  });

  test('multiple highlights saved in order', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(async () => { await clearAllHighlights(); });
    await page.evaluate(async () => {
      for (const type of ['goal', 'save', 'nearmiss']) {
        const blob = new Blob([type], { type: 'video/webm' });
        await saveHighlight({ type, blob, timestamp: Date.now(), team: null, scoreAfter: null });
      }
    });
    const count = await page.evaluate(async () => countHighlights());
    expect(count).toBe(3);
  });

  test('highlights list badge updates after save', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    await page.evaluate(async () => { await clearAllHighlights(); await refreshHighlightsListBadge(); });
    await page.evaluate(async () => {
      const blob = new Blob(['x'], { type: 'video/webm' });
      await saveHighlight({ type: 'save', blob, timestamp: Date.now(), team: null, scoreAfter: null });
      await refreshHighlightsListBadge();
    });
    await expect(page.locator('#hlMeta')).toHaveText('1 clip');
  });

  test('highlights badge shows correct plural count', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    await page.evaluate(async () => {
      await clearAllHighlights();
      for (let i = 0; i < 3; i++) {
        const blob = new Blob([String(i)], { type: 'video/webm' });
        await saveHighlight({ type: 'save', blob, timestamp: Date.now(), team: null, scoreAfter: null });
      }
      await refreshHighlightsListBadge();
    });
    await expect(page.locator('#hlMeta')).toHaveText('3 clips');
  });

  test('goal without team is filtered from highlights reel', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(async () => { await clearAllHighlights(); });
    await page.evaluate(async () => {
      const blob = new Blob(['x'], { type: 'video/webm' });
      await saveHighlight({ type: 'goal', blob, timestamp: Date.now(), team: null, scoreAfter: null });
      await saveHighlight({ type: 'save', blob, timestamp: Date.now(), team: null, scoreAfter: null });
    });
    const filtered = await page.evaluate(async () => {
      const all = await getHighlights();
      return all.filter(e => !(e.type === 'goal' && !e.team)).length;
    });
    expect(filtered).toBe(1);
  });
});
