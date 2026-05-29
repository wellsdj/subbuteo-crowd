const { test, expect } = require('@playwright/test');
const { gotoApp, navigateTo } = require('./helpers');

test.describe('Highlights UI', () => {
  test('highlights section exists', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#highlightsSection')).toBeAttached();
  });

  test('play highlights button is visible on load', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    await expect(page.locator('#hlPlayBtn')).toBeVisible();
  });

  test('stop button hidden when not playing', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    const display = await page.locator('#hlStopBtn').evaluate(el => el.style.display);
    expect(display).toBe('none');
  });

  test('mic button is present', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    await expect(page.locator('#hlMicBtn')).toBeVisible();
  });

  test('mic button default text shows MIC ON', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    await expect(page.locator('#hlMicBtn')).toContainText('MIC ON');
  });

  test('toggleHlMic changes button to MIC OFF', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    await page.evaluate(() => toggleHlMic());
    await expect(page.locator('#hlMicBtn')).toContainText('MIC OFF');
  });

  test('toggleHlMic toggles back to MIC ON', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    await page.evaluate(() => { toggleHlMic(); toggleHlMic(); });
    await expect(page.locator('#hlMicBtn')).toContainText('MIC ON');
  });

  test('hl video element exists', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#hlVideo')).toBeAttached();
  });

  test('hl bottom board exists', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#hlBottomBoard')).toBeAttached();
  });

  test('hl bottom board hidden initially', async ({ page }) => {
    await gotoApp(page);
    const display = await page.locator('#hlBottomBoard').evaluate(el => el.style.display);
    expect(display).toBe('none');
  });

  test('hl title card exists', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#hlTitleCard')).toBeAttached();
  });

  test('hl goal flash element exists', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#hlGoalFlash')).toBeAttached();
  });

  test('hl transition element exists', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#hlTransition')).toBeAttached();
  });

  test('hl meta shows 0 clips when empty', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    await page.evaluate(async () => {
      await clearAllHighlights();
      await refreshHighlightsListBadge();
    });
    await expect(page.locator('#hlMeta')).toHaveText('0 clips');
  });

  test('final score element hidden initially', async ({ page }) => {
    await gotoApp(page);
    const display = await page.locator('#hlFinalScore').evaluate(el => el.style.display);
    expect(display).toBe('none');
  });

  test('stopHighlights hides stop button', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    await page.evaluate(() => {
      document.getElementById('hlStopBtn').style.display = '';
      stopHighlights();
    });
    const display = await page.locator('#hlStopBtn').evaluate(el => el.style.display);
    expect(display).toBe('none');
  });

  test('stopHighlights shows play button', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    await page.evaluate(() => {
      document.getElementById('hlPlayBtn').style.display = 'none';
      stopHighlights();
    });
    await expect(page.locator('#hlPlayBtn')).toBeVisible();
  });

  test('highlights section is hidden when navigating away', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    await navigateTo(page, 'crowd');
    const opacity = await page.locator('#highlightsSection').evaluate(el => el.style.opacity);
    expect(opacity).toBe('0');
  });
});
