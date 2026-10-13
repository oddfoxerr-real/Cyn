"""Add a user-triggered Spotify session export to an existing EMBER panel copy."""
from pathlib import Path
import sys

SCRIPT = r'''
<script id="cyn-spotify-session-export">
(() => {
  function install() {
    const anchor = document.getElementById('spotifyLogout');
    if (!anchor || document.getElementById('spotifyExportCyn')) return;
    const button = document.createElement('button');
    button.id = 'spotifyExportCyn';
    button.className = 'spotify-ghost';
    button.textContent = 'Export session for Cyn';
    button.title = 'Save your existing Spotify connection as a private JSON file for Cyn';
    button.onclick = () => {
      let tokens;
      try { tokens = JSON.parse(localStorage.getItem('ember2_spotify_tokens') || 'null'); }
      catch { tokens = null; }
      if (!tokens?.access_token && !tokens?.refresh_token) {
        const status = document.getElementById('spotifyStatus');
        if (status) status.textContent = 'Connect Spotify in EMBER before exporting for Cyn';
        return;
      }
      const payload = { format: 'ember-spotify-session', version: 1,
        client_id: 'c82b14bfdfa24024ab295548bd169aae', tokens };
      const url = URL.createObjectURL(new Blob([JSON.stringify(payload, null, 2)], {type:'application/json'}));
      const link = document.createElement('a');
      link.href = url; link.download = 'ember-spotify.json'; link.click();
      setTimeout(() => URL.revokeObjectURL(url), 1000);
      const status = document.getElementById('spotifyStatus');
      if (status) status.textContent = 'Session exported. Keep this JSON private and select it in Cyn.';
    };
    anchor.insertAdjacentElement('afterend', button);
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', install);
  else install();
})();
</script>
'''

if __name__ == '__main__':
    source, target = map(Path, sys.argv[1:3])
    text = source.read_text(encoding='utf-8-sig')
    if 'cyn-spotify-session-export' not in text:
        assert 'ember2_spotify_tokens' in text and 'spotifyLogout' in text
        at = text.lower().rfind('</body>')
        text = text[:at] + SCRIPT + text[at:] if at >= 0 else text + SCRIPT
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text, encoding='utf-8')
    print(f'Prepared export-enabled EMBER panel: {target}')
