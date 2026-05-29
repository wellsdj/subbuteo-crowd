/**
 * Shared helpers for Subbuteo Crowd tests.
 * Bypasses the auth splash by injecting sessionStorage before page load.
 */

async function bypassAuth(page) {
  await page.addInitScript(() => {
    sessionStorage.setItem('crowdAuth', '1');
    // Suppress audio context + getUserMedia so tests don't need real devices
    window.__testMode = true;
  });
}

async function gotoApp(page) {
  await bypassAuth(page);
  await page.goto('/');
  // Wait for DOMContentLoaded-driven showSection('crowd') to complete
  await page.waitForSelector('.app', { state: 'visible', timeout: 10000 });
}

async function navigateTo(page, section) {
  // Call showSection() directly — the nav sidebar is closed by default so
  // clicking nav items times out waiting for visibility.
  await page.evaluate(s => showSection(s), section);
  await page.waitForTimeout(80);
}

module.exports = { bypassAuth, gotoApp, navigateTo };
