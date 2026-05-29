const { test, expect } = require('@playwright/test');
const { gotoApp } = require('./helpers');

// Navigation helper: call showSection() directly rather than clicking
// the sidebar button (which is hidden when the drawer is closed).
async function nav(page, section) {
  await page.evaluate(s => showSection(s), section);
  await page.waitForTimeout(80);
}

test.describe('Navigation', () => {
  test('clicking Match nav shows match section', async ({ page }) => {
    await gotoApp(page);
    await nav(page, 'match');
    await expect(page.locator('#matchSection')).toHaveClass(/active/);
  });

  test('clicking Highlights nav shows highlights section', async ({ page }) => {
    await gotoApp(page);
    await nav(page, 'highlights');
    await expect(page.locator('#highlightsSection')).toHaveClass(/active/);
  });

  test('clicking Replay nav shows replay section', async ({ page }) => {
    await gotoApp(page);
    await nav(page, 'replay');
    await expect(page.locator('#replaySection')).toHaveClass(/active/);
  });

  test('navigating away from crowd hides app panel', async ({ page }) => {
    await gotoApp(page);
    await nav(page, 'match');
    const opacity = await page.locator('.app').evaluate(el => el.style.opacity);
    expect(opacity).toBe('0');
  });

  test('clicking nav crowd returns to crowd view', async ({ page }) => {
    await gotoApp(page);
    await nav(page, 'match');
    await nav(page, 'crowd');
    const opacity = await page.locator('.app').evaluate(el => el.style.opacity);
    expect(opacity).toBe('1');
  });

  test('banner hidden on highlights section', async ({ page }) => {
    await gotoApp(page);
    await nav(page, 'highlights');
    await expect(page.locator('#matchBanner')).toHaveClass(/hidden/);
  });

  test('banner visible on match section', async ({ page }) => {
    await gotoApp(page);
    await nav(page, 'match');
    await expect(page.locator('#matchBanner')).not.toHaveClass(/hidden/);
  });

  test('banner visible on crowd section', async ({ page }) => {
    await gotoApp(page);
    // Already on crowd
    await expect(page.locator('#matchBanner')).not.toHaveClass(/hidden/);
  });

  test('nav item gets active class when section selected', async ({ page }) => {
    await gotoApp(page);
    await nav(page, 'match');
    await expect(page.locator('#navMatch')).toHaveClass(/active/);
    await expect(page.locator('#navCrowd')).not.toHaveClass(/active/);
  });

  test('hamburger opens nav sidebar', async ({ page }) => {
    await gotoApp(page);
    await page.click('[onclick="openNav()"]');
    await expect(page.locator('#navSidebar')).toHaveClass(/open/);
  });
});
