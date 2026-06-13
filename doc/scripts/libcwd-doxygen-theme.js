// Keep the Doxygen Awesome light/dark preference coherent across the generated libcwd documentation.
//
// Doxygen Awesome persists the toggle in localStorage, which works for an HTTP-served documentation tree but is
// browser-dependent for file:// pages. Carry the explicit libcwd choice in generated documentation links as well, so
// local file navigation keeps the selected theme even when each file has isolated storage.
(function() {
  'use strict';

  const preferenceKey = 'libcwd-color-scheme';
  const urlPreferenceKey = 'libcwd-theme';
  const darkPreference = 'dark';
  const lightPreference = 'light';

  // Read a persisted explicit preference.
  //
  // Returns "dark", "light", or null when no libcwd-specific preference has been recorded. The URL value is preferred
  // over localStorage so a click from one file:// page overrides stale per-page localStorage values on the destination
  // page.
  function readStoredPreference() {
    const urlPreference = readUrlPreference(window.location.href);
    if (urlPreference !== null) {
      return urlPreference;
    }

    try {
      const storagePreference = window.localStorage.getItem(preferenceKey);
      return isPreference(storagePreference) ? storagePreference : null;
    } catch (error) {
      return null;
    }
  }

  // Store an explicit preference in origin storage and mirror it into same-documentation links.
  //
  // localStorage gives persistent behavior when the documentation is served normally. Link propagation gives coherent
  // behavior while browsing a local generated documentation tree through file:// URLs.
  function writeStoredPreference(preference) {
    try {
      window.localStorage.setItem(preferenceKey, preference);
    } catch (error) {
      // Some browsers disable localStorage for local files or privacy modes; link propagation still carries navigation.
    }

    replaceCurrentUrlPreference(preference);
    updateDocumentationLinks(preference);
  }

  // Return true when value is one of the explicit theme preference strings.
  //
  // This keeps URL and storage parsing strict so unrelated query parameters never change the selected theme.
  function isPreference(value) {
    return value === darkPreference || value === lightPreference;
  }

  // Extract the libcwd preference from a URL.
  //
  // Returns null for invalid URLs or when the query string does not carry an explicit libcwd theme.
  function readUrlPreference(url) {
    try {
      const parsedUrl = new URL(url, window.location.href);
      const preference = parsedUrl.searchParams.get(urlPreferenceKey);
      return isPreference(preference) ? preference : null;
    } catch (error) {
      return null;
    }
  }

  // Return a URL string with the libcwd preference set in the query string.
  //
  // Existing query parameters and fragments are preserved. Invalid or unsupported links are returned unchanged.
  function withUrlPreference(url, preference) {
    try {
      const parsedUrl = new URL(url, window.location.href);
      parsedUrl.searchParams.set(urlPreferenceKey, preference);
      return parsedUrl.href;
    } catch (error) {
      return url;
    }
  }

  // Update the current address bar with the active preference when the History API is available.
  //
  // This makes refreshes preserve the explicit choice and ensures copied URLs from the active page carry the theme.
  function replaceCurrentUrlPreference(preference) {
    if (!window.history || typeof window.history.replaceState !== 'function') {
      return;
    }

    try {
      window.history.replaceState(window.history.state, document.title, withUrlPreference(window.location.href, preference));
    } catch (error) {
      // Some browsers restrict history changes for file:// URLs. Link rewriting still preserves normal navigation.
    }
  }

  // Return true for ordinary documentation links that should carry the theme preference.
  //
  // External links, non-HTTP/file protocols, JavaScript pseudo-links, and Doxygen's search iframe link are left alone.
  function shouldPropagateToLink(anchor) {
    const rawHref = anchor.getAttribute('href');
    if (!rawHref || rawHref.startsWith('#') || rawHref.startsWith('javascript:')) {
      return false;
    }

    if (anchor.target && anchor.target !== '_self') {
      return false;
    }

    let parsedUrl;
    try {
      parsedUrl = new URL(rawHref, window.location.href);
    } catch (error) {
      return false;
    }

    if (parsedUrl.protocol !== 'file:' && parsedUrl.protocol !== 'http:' && parsedUrl.protocol !== 'https:') {
      return false;
    }

    if (parsedUrl.origin !== window.location.origin) {
      return false;
    }

    const path = parsedUrl.pathname;
    return path.endsWith('/') || path.endsWith('.html') || path.endsWith('.htm');
  }

  // Mirror the active theme into all currently available documentation links.
  //
  // This covers side-nav links, in-page Doxygen links, and the top-level generated menu. Dynamic Doxygen fragments are
  // also handled by the click listener, so a missed late-added link is updated just before navigation.
  function updateDocumentationLinks(preference) {
    if (!document.querySelectorAll) {
      return;
    }

    for (const anchor of document.querySelectorAll('a[href]')) {
      if (shouldPropagateToLink(anchor)) {
        anchor.href = withUrlPreference(anchor.href, preference);
      }
    }
  }

  // Ensure the link that is about to be followed carries the current explicit preference.
  //
  // Capturing the click handles links inserted after the initial page load without changing Doxygen's click handlers.
  function installClickPropagation() {
    document.addEventListener('click', event => {
      const anchor = event.target && event.target.closest ? event.target.closest('a[href]') : null;
      if (!anchor || !shouldPropagateToLink(anchor)) {
        return;
      }

      const preference = readStoredPreference();
      if (preference !== null) {
        anchor.href = withUrlPreference(anchor.href, preference);
      }
    }, true);
  }

  // Run a callback once the document body and generated links are available.
  //
  // The script is loaded from the HTML head so immediate link rewriting is too early for normal page anchors.
  function whenDocumentReady(callback) {
    if (document.readyState === 'loading') {
      document.addEventListener('DOMContentLoaded', callback, { once: true });
    } else {
      callback();
    }
  }

  // Extend Doxygen Awesome after its class has been loaded, but before libcwd calls its init() hook.
  //
  // The original class still owns all DOM changes and icon handling. This wrapper only replaces the preference
  // accessor so every page reads and writes the same explicit libcwd choice.
  function installSharedThemePreference() {
    const toggleClass = typeof DoxygenAwesomeDarkModeToggle === 'undefined' ? null : DoxygenAwesomeDarkModeToggle;
    if (!toggleClass || toggleClass.__libcwdSharedThemePreferenceInstalled) {
      return;
    }

    const originalUserPreference = Object.getOwnPropertyDescriptor(toggleClass, 'userPreference');
    if (!originalUserPreference || typeof originalUserPreference.get !== 'function') {
      return;
    }

    Object.defineProperty(toggleClass, 'userPreference', {
      configurable: true,
      get() {
        const preference = readStoredPreference();
        if (preference === 'dark') {
          return true;
        }
        if (preference === 'light') {
          return false;
        }
        return originalUserPreference.get.call(toggleClass);
      },
      set(enableDarkMode) {
        const preference = enableDarkMode ? 'dark' : 'light';
        writeStoredPreference(preference);
        toggleClass.darkModeEnabled = Boolean(enableDarkMode);
        toggleClass.onUserPreferenceChanged();
      }
    });

    toggleClass.__libcwdSharedThemePreferenceInstalled = true;
    const preference = readStoredPreference();
    if (preference !== null) {
      replaceCurrentUrlPreference(preference);
      whenDocumentReady(() => updateDocumentationLinks(preference));
    }
    toggleClass.enableDarkMode(toggleClass.userPreference);
  }

  installClickPropagation();
  installSharedThemePreference();
})();
