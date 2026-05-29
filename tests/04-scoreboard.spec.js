const { test, expect } = require('@playwright/test');
const { gotoApp, navigateTo } = require('./helpers');

test.describe('Scoreboard', () => {
  test('home score starts at 0', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await expect(page.locator('#msHomeScore')).toHaveText('0');
  });

  test('away score starts at 0', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await expect(page.locator('#msAwayScore')).toHaveText('0');
  });

  test('home +1 button increments home score', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await page.evaluate(() => changeScore('home', 1));
    await expect(page.locator('#msHomeScore')).toHaveText('1');
  });

  test('away +1 button increments away score', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await page.evaluate(() => changeScore('away', 1));
    await expect(page.locator('#msAwayScore')).toHaveText('1');
  });

  test('home -1 decrements home score', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await page.evaluate(() => { changeScore('home', 1); changeScore('home', -1); });
    await expect(page.locator('#msHomeScore')).toHaveText('0');
  });

  test('score cannot go below 0', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await page.evaluate(() => changeScore('home', -5));
    await expect(page.locator('#msHomeScore')).toHaveText('0');
  });

  test('banner score updates when home scores', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => changeScore('home', 1));
    await expect(page.locator('#bannerScore')).toHaveText('1 — 0');
  });

  test('banner score updates when away scores', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => changeScore('away', 2));
    await expect(page.locator('#bannerScore')).toHaveText('0 — 2');
  });

  test('banner shows correct score after both teams score', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => { changeScore('home', 3); changeScore('away', 1); });
    await expect(page.locator('#bannerScore')).toHaveText('3 — 1');
  });

  test('addToScoreboard home increases home score by 1', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      window.pendingHighlightId = null;
      addToScoreboard('home');
    });
    await expect(page.locator('#msHomeScore')).toHaveText('1');
  });

  test('addToScoreboard null does not change score', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      window.pendingHighlightId = null;
      addToScoreboard(null);
    });
    await expect(page.locator('#msHomeScore')).toHaveText('0');
    await expect(page.locator('#msAwayScore')).toHaveText('0');
  });
});
