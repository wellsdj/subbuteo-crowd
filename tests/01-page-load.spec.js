const { test, expect } = require('@playwright/test');
const { gotoApp } = require('./helpers');

test.describe('Page load and structure', () => {
  test('page title is set', async ({ page }) => {
    await gotoApp(page);
    const title = await page.title();
    expect(title).toBeTruthy();
  });

  test('splash is hidden after auth bypass', async ({ page }) => {
    await gotoApp(page);
    const splash = page.locator('#splash');
    await expect(splash).toHaveCSS('display', 'none');
  });

  test('crowd section visible on load', async ({ page }) => {
    await gotoApp(page);
    const app = page.locator('.app');
    await expect(app).toBeVisible();
  });

  test('match banner is rendered', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#matchBanner')).toBeAttached();
  });

  test('banner shows 0 — 0 initially', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#bannerScore')).toHaveText('0 — 0');
  });

  test('banner shows 30:00 initially', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#bannerTimer')).toHaveText('30:00');
  });

  test('nav sidebar has 5 sections', async ({ page }) => {
    await gotoApp(page);
    const navItems = page.locator('.nav-item');
    await expect(navItems).toHaveCount(5);
  });

  test('nav crowd button is active on load', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#navCrowd')).toHaveClass(/active/);
  });

  test('replay popup hidden initially', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#replayPopup')).not.toHaveClass(/visible/);
  });

  test('goal popup hidden initially', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#goalPopup')).not.toHaveClass(/visible/);
  });

  test('score prompt hidden initially', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#scorePrompt')).not.toHaveClass(/visible/);
  });
});
