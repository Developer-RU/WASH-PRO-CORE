document.addEventListener('DOMContentLoaded', ()=>{
  const toggle = document.getElementById('toggle');
  const sidebar = document.getElementById('sidebar');

  function updateIcon() {
    if (sidebar.classList.contains('open')) {
      toggle.classList.add('is-active');
    } else {
      toggle.classList.remove('is-active');
    }
  }

  toggle.addEventListener('click', () => {
    sidebar.classList.toggle('open');
    updateIcon();
  });

  document.querySelectorAll('.sidebar ul li').forEach(li=>{
    li.addEventListener('click', ()=>{
      document.querySelectorAll('.sidebar ul li').forEach(x=>x.classList.remove('active'));
      li.classList.add('active');
      document.querySelectorAll('.page').forEach(p=>p.classList.remove('active'));
      const page = document.getElementById(li.dataset.page);
      if (page) page.classList.add('active');
      document.getElementById('title').innerText = li.innerText;
      if (li.dataset.page === 'home') loadInfo();
      if (li.dataset.page === 'tasks') loadTasksEnhanced();
      if (li.dataset.page === 'system') loadSystem();
      // Hide sidebar after navigation on small screens
      if (sidebar.classList.contains('open')) {
        sidebar.classList.remove('open');
        updateIcon();
      }
    });
  });

  // Close sidebar when clicking outside of it (mobile-friendly)
  document.addEventListener('click', (e)=>{
    if (!sidebar.classList.contains('open')) return;
    const target = e.target;
    if (toggle.contains(target)) return; // toggle handled separately
    if (!sidebar.contains(target)) {
      sidebar.classList.remove('open');
      updateIcon();
    }
  });

  updateIcon();

  function loadInfo(){
    fetch('/api/info').then(r=>r.json()).then(j=>{
      document.getElementById('info').innerHTML = `<table>
        <tr><td>Serial</td><td>${j.serial}</td></tr>
        <tr><td>License active</td><td>${j.licenseActive}</td></tr>
        <tr><td>Free heap</td><td>${j.freeHeap}</td></tr>
        <tr><td>Heap size</td><td>${j.heapSize}</td></tr>
        <tr><td>User scripts</td><td>${j.userScripts}</td></tr>
        <tr><td>Running tasks</td><td>${j.runningTasks}</td></tr>
      </table>`;
    });
  }

  function loadTasks(){
    fetch('/api/tasks').then(r=>r.json()).then(j=>{
      const ul = document.getElementById('tasksList'); ul.innerHTML = '';
      j.forEach(t=>{
        const li = document.createElement('li');
        li.textContent = `${t.name} — ${t.state}`;
        ul.appendChild(li);
      });
    });
  }

  function loadSystem(){
    fetch('/api/system').then(r=>r.json()).then(j=>{
      document.getElementById('systemInfo').innerHTML = `<table>
        <tr><td>SW</td><td>${j.swSerial || ''}</td></tr>
        <tr><td>Theme</td><td>${j.theme || ''}</td></tr>
        <tr><td>Language</td><td>${j.language || ''}</td></tr>
      </table>`;
      document.getElementById('language').value = j.language || 'en';
      // Apply theme from device
      const theme = j.theme || 'gp_light';
      const themeLink = document.getElementById('theme');
      if (themeLink) themeLink.href = `/themes/${theme}.css`;
      // populate theme select if present
      const sel = document.getElementById('themeSelect');
      if (sel) sel.value = theme;
    });
  }

  // load available themes and populate selector
  function loadThemes(){
    fetch('/api/themes').then(r=>r.json()).then(list=>{
      const sel = document.getElementById('themeSelect');
      if (!sel) return;
      sel.innerHTML = '';
      list.forEach(name=>{
        const opt = document.createElement('option'); opt.value = name; opt.textContent = name.replace('gp_','').replace(/_/g,' ');
        sel.appendChild(opt);
      });
      // set current value after populating
      fetch('/api/system').then(r=>r.json()).then(j=>{ sel.value = j.theme || 'gp_light'; });
    }).catch(()=>{
      // fallback if endpoint missing: basic set
      const sel = document.getElementById('themeSelect'); if (sel && sel.options.length===0){ ['gp_light','gp_dark'].forEach(n=>{ const o=document.createElement('option'); o.value=n; o.textContent=n; sel.appendChild(o); }); }
    });
  }

  // language & translations support
  const LANGS = [
    {code:'en', name:'English'},{code:'ru', name:'Русский'},{code:'es', name:'Español'},{code:'fr', name:'Français'},{code:'de', name:'Deutsch'},{code:'it', name:'Italiano'},{code:'pt', name:'Português'},{code:'nl', name:'Nederlands'},{code:'pl', name:'Polski'},{code:'sv', name:'Svenska'},{code:'da', name:'Dansk'},{code:'fi', name:'Suomi'},{code:'ja', name:'日本語'},{code:'ko', name:'한국어'},{code:'zh', name:'中文'},{code:'ar', name:'العربية'}
  ];
  let TRANSLATIONS = {};

  function populateLanguageSelect(){
    const sel = document.getElementById('language'); if (!sel) return;
    sel.innerHTML = '';
    LANGS.forEach(l=>{ const o=document.createElement('option'); o.value = l.code; o.textContent = l.name; sel.appendChild(o); });
  }

  async function loadTranslations(lang){
    try{
      const r = await fetch(`/lang/${lang}.json`);
      if (r.ok){ TRANSLATIONS = await r.json(); applyTranslations(); }
      else console.warn('Translation not found for', lang);
    }catch(e){ console.warn('Failed to load translations', e); }
  }

  function applyTranslations(){
    const t = TRANSLATIONS || {};
    document.querySelector('nav .brand').textContent = t.brand || document.querySelector('nav .brand').textContent;
    document.querySelector('nav ul li[data-page="home"] span').textContent = t.nav?.home || 'Home';
    document.querySelector('nav ul li[data-page="tasks"] span').textContent = t.nav?.tasks || 'Tasks';
    document.querySelector('nav ul li[data-page="system"] span').textContent = t.nav?.system || 'System';
    document.querySelector('nav ul li[data-page="wifi"] span').textContent = t.nav?.wifi || 'Wi‑Fi';
    document.querySelector('nav ul li[data-page="reboot"] span').textContent = t.nav?.reboot || 'Reboot';

    document.getElementById('infoTitle').textContent = t.info?.title || 'Information';
    document.getElementById('tasksTitle').textContent = t.tasks?.title || 'Tasks';
    document.getElementById('systemTitle').textContent = t.system?.title || 'System';
    document.getElementById('fwTitle').textContent = t.system?.fw || (t.system?.title || 'Firmware / Filesystem');
    document.getElementById('wifiTitle').textContent = t.wifi?.title || 'Wireless network';
    document.getElementById('rebootTitle').textContent = t.reboot?.title || 'Reboot';
    document.getElementById('themeTitle').textContent = t.saveTheme?.title || 'Appearance';

    document.getElementById('createTaskBtn').querySelector('.btn-text').textContent = t.tasks?.create || 'Create task';
    document.getElementById('uploadFirmwareBtn').querySelector('.btn-text').textContent = t.firmware?.uploadButton || 'Upload firmware (OTA)';
    document.getElementById('uploadFSBtn').querySelector('.btn-text').textContent = t.fs?.uploadButton || 'Upload to filesystem';
    document.getElementById('saveLang').querySelector('.btn-text').textContent = t.save?.button || 'Save';
    document.getElementById('saveTheme').querySelector('.btn-text').textContent = t.saveTheme?.button || 'Save theme';
    document.getElementById('wifiSaveBtn').querySelector('.btn-text').textContent = t.wifi?.saveButton || 'Save and connect';
    document.getElementById('rebootBtn').querySelector('.btn-text').textContent = t.reboot?.button || 'Reboot';
    document.getElementById('saveScriptBtn').querySelector('.btn-text').textContent = t.save?.button || 'Save';

    const ssid = document.querySelector('#wifiForm input[name="ssid"]'); if (ssid) ssid.placeholder = t.wifi?.ssidPlaceholder || 'SSID';
    const pass = document.querySelector('#wifiForm input[name="pass"]'); if (pass) pass.placeholder = t.wifi?.passPlaceholder || 'Password';

    const rtype = document.getElementById('rebootTypeLabel'); if (rtype) rtype.textContent = t.reboot?.typeLabel || 'Type:';
    const rdelay = document.getElementById('rebootDelayLabel'); if (rdelay) rdelay.textContent = t.reboot?.delayLabel || 'Delay (sec):';
    const typeSel = document.querySelector('#rebootForm select[name="type"]');
    if (typeSel){ typeSel.querySelector('option[value="soft"]').textContent = t.reboot?.typeSoft || 'Soft'; typeSel.querySelector('option[value="hard"]').textContent = t.reboot?.typeHard || 'Hard'; }

    if (document.getElementById('home').classList.contains('active')) loadInfo();
    if (document.getElementById('system').classList.contains('active')) loadSystem();
  }

  function loadInfo(){
    fetch('/api/info').then(r=>r.json()).then(j=>{
      const t = TRANSLATIONS.info || {};
      document.getElementById('info').innerHTML = `<table>
        <tr><td>${t.serial || 'Serial'}</td><td>${j.serial || ''}</td></tr>
        <tr><td>${t.licenseActive || 'License active'}</td><td>${j.licenseActive || ''}</td></tr>
        <tr><td>${t.freeHeap || 'Free heap'}</td><td>${j.freeHeap || ''}</td></tr>
        <tr><td>${t.heapSize || 'Heap size'}</td><td>${j.heapSize || ''}</td></tr>
        <tr><td>${t.userScripts || 'User scripts'}</td><td>${j.userScripts || ''}</td></tr>
        <tr><td>${t.runningTasks || 'Running tasks'}</td><td>${j.runningTasks || ''}</td></tr>
      </table>`;
    });
  }

  function loadTasks(){
    fetch('/api/tasks').then(r=>r.json()).then(j=>{
      const ul = document.getElementById('tasksList'); ul.innerHTML = '';
      j.forEach(t=>{
        const li = document.createElement('li');
        li.textContent = `${t.name} — ${t.state}`;
        ul.appendChild(li);
      });
    });
  }

  function loadSystem(){
    fetch('/api/system').then(r=>r.json()).then(j=>{
      const t = TRANSLATIONS.system || {};
      document.getElementById('systemInfo').innerHTML = `<table>
        <tr><td>${t.sw || 'SW'}</td><td>${j.swSerial || ''}</td></tr>
        <tr><td>${t.theme || 'Theme'}</td><td>${j.theme || ''}</td></tr>
        <tr><td>${t.language || 'Language'}</td><td>${j.language || ''}</td></tr>
      </table>`;

      const lang = j.language || 'en';
      const theme = j.theme || 'gp_light';
      document.getElementById('language').value = lang;
      const themeLink = document.getElementById('theme'); if (themeLink) themeLink.href = `/themes/${theme}.css`;
      const sel = document.getElementById('themeSelect'); if (sel) sel.value = theme;

      loadTranslations(lang);
    });
  }

  // load available themes and populate selector
  function loadThemes(){
    fetch('/api/themes').then(r=>r.json()).then(list=>{
      const sel = document.getElementById('themeSelect');
      if (!sel) return;
      sel.innerHTML = '';
      list.forEach(name=>{ const opt = document.createElement('option'); opt.value = name; opt.textContent = name.replace('gp_','').replace(/_/g,' '); sel.appendChild(opt); });
      // set current value after populating
      fetch('/api/system').then(r=>r.json()).then(j=>{ sel.value = j.theme || 'gp_light'; });
    }).catch(()=>{
      const sel = document.getElementById('themeSelect'); if (sel && sel.options.length===0){ ['gp_light','gp_dark'].forEach(n=>{ const o=document.createElement('option'); o.value=n; o.textContent=n; sel.appendChild(o); }); }
    });
  }

        // Enhanced tasks UI and script editor
    async function loadTasksEnhanced(){
      fetch('/api/tasks').then(r=>r.json()).then(j=>{
        const container = document.getElementById('tasksList'); container.innerHTML = '';
        const arr = (j && j.tasks) ? j.tasks : (Array.isArray(j) ? j : []);
        arr.forEach(t=>{
          const card = document.createElement('div'); card.className = 'task-card';
          const info = document.createElement('div'); info.className = 'task-info';
          const title = document.createElement('div'); title.textContent = t.name; title.style.fontWeight='700';
          const meta = document.createElement('div'); meta.textContent = `State: ${t.state || ''} ${t.hasScript? ' • has script':''}`;
          info.appendChild(title); info.appendChild(meta);
          const actions = document.createElement('div'); actions.className='task-actions';
          // helper to create an icon+label button
          function makeAction(icon, labelText, cb){
            const b = document.createElement('button');
            b.className = 'task-action-btn';
            b.textContent = labelText;
            b.title = labelText;
            b.setAttribute('aria-label', labelText);
            b.addEventListener('click', cb);
            return b;
          }
  
          const editLabel = TRANSLATIONS.tasks?.edit || 'Edit';
          const editBtn = makeAction('<i class="fas fa-pencil"></i>', editLabel, async ()=>{
            const newName = prompt(TRANSLATIONS.tasks?.renamePrompt || 'New name', t.name);
            if (!newName) return;
            const r = await fetch('/api/tasks', { method:'POST', body: new URLSearchParams({name:newName, id:t.id}) });
            if (r.ok) loadTasksEnhanced();
          });
  
          const scriptLabel = t.hasScript? (TRANSLATIONS.tasks?.editScript||'Edit script') : (TRANSLATIONS.tasks?.attachScript||'Attach script');
          const scriptIcon = t.hasScript ? '<i class="fas fa-file-lines"></i>' : '<i class="fas fa-paperclip"></i>';
          const scriptBtn = makeAction(scriptIcon, scriptLabel, ()=>{ openScriptEditor(t.id, t.name); });
  
          const delLabel = TRANSLATIONS.tasks?.delete || 'Delete';
          const delBtn = makeAction('<i class="fas fa-trash-can"></i>', delLabel, async ()=>{ if (!confirm(TRANSLATIONS.tasks?.deleteConfirm || 'Delete task?')) return; await fetch('/api/tasks/delete', { method:'POST', body: new URLSearchParams({id:t.id}) }); loadTasksEnhanced(); });
  
          const runLabel = TRANSLATIONS.tasks?.run || 'Run';
          const runBtn = makeAction('<i class="fas fa-play"></i>', runLabel, async ()=>{ await fetch('/api/tasks/run', { method:'POST', body: new URLSearchParams({id: t.id}) }); alert(TRANSLATIONS.tasks?.runStarted || 'Run requested'); });
  
          actions.appendChild(editBtn); actions.appendChild(scriptBtn); actions.appendChild(runBtn); actions.appendChild(delBtn);
          card.appendChild(info); card.appendChild(actions);
          container.appendChild(card);
        });
      });
    }
  // Create task button
  document.getElementById('createTaskBtn')?.addEventListener('click', async ()=>{
    const name = prompt(TRANSLATIONS.tasks?.createPrompt || 'Task name'); if (!name) return;
    const r = await fetch('/api/tasks', { method:'POST', body: new URLSearchParams({name}) });
    if (r.ok) loadTasksEnhanced();
  });

  // Script editor utilities
  let currentEditingTask = null;
  async function openScriptEditor(taskId, taskName){
    currentEditingTask = taskId;
    document.getElementById('editorTitle').textContent = `${TRANSLATIONS.tasks?.scriptFor || 'Script:'} ${taskName || taskId}`;
    document.getElementById('scriptContent').value = '';
    // load existing script
    const r = await fetch(`/api/tasks/script?id=${encodeURIComponent(taskId)}`);
    if (r.ok){ const txt = await r.text(); document.getElementById('scriptContent').value = txt; }
    // load builtins
    fetch('/api/builtins').then(r=>r.json()).then(list=>{
      const ul = document.getElementById('builtinList'); ul.innerHTML = '';
      list.forEach(fn=>{ const li = document.createElement('li'); li.textContent = fn; li.addEventListener('click', ()=>{ insertAtCursor(document.getElementById('scriptContent'), fn + '()'); }); ul.appendChild(li); });
    });
    document.getElementById('scriptEditor').style.display = 'flex';
  }
  document.getElementById('closeEditor')?.addEventListener('click', ()=>{ document.getElementById('scriptEditor').style.display = 'none'; currentEditingTask = null; });
  document.getElementById('saveScriptBtn')?.addEventListener('click', async ()=>{
    if (!currentEditingTask) return;
    const content = document.getElementById('scriptContent').value;
    // Send JSON payload to avoid multipart/parser issues
    const payload = JSON.stringify({ id: currentEditingTask, script: content });
    console.log('POST /api/tasks/script JSON', {id: currentEditingTask, len: content.length});
    const r = await fetch('/api/tasks/script', { method:'POST', headers: {'Content-Type':'application/json'}, body: payload });
    const txt = await r.text();
    if (r.ok){ alert(TRANSLATIONS.alerts?.saved || 'Saved'); document.getElementById('scriptEditor').style.display='none'; loadTasksEnhanced(); }
    else { console.error('Save script failed:', r.status, txt); alert('Failed to save: ' + txt); }
  });

  function insertAtCursor(el, text){
    const start = el.selectionStart || 0; const end = el.selectionEnd || 0; const v = el.value;
    el.value = v.slice(0,start) + text + v.slice(end);
    el.selectionStart = el.selectionEnd = start + text.length;
    el.focus();
  }

  document.getElementById('saveLang').addEventListener('click', async ()=>{
    const lang = document.getElementById('language').value;
    const r = await fetch('/api/setlanguage', { method:'POST', body: new URLSearchParams({lang}) });
    if (r.ok){ await loadTranslations(lang); alert(TRANSLATIONS.alerts?.saved || 'Saved'); }
  });

  // theme save
  document.getElementById('saveTheme')?.addEventListener('click', async ()=>{
    const theme = document.getElementById('themeSelect').value;
    const r = await fetch('/api/settheme', { method:'POST', body: new URLSearchParams({theme}) });
    if (r.ok){ document.getElementById('theme').href = `/themes/${theme}.css`; alert(TRANSLATIONS.alerts?.themeSaved || 'Theme saved'); }
  });

  // uploads
  document.getElementById('uploadFirmware').addEventListener('submit', async (e)=>{
    e.preventDefault();
    const fd = new FormData(e.target);
    const file = e.target.querySelector('input[name="firmware"]').files[0];
    if (!file) return alert(TRANSLATIONS.alerts?.selectFile || 'Select file');
    const r = await fetch('/api/upload/firmware', { method:'POST', body: fd });
    if (r.ok) alert(TRANSLATIONS.alerts?.uploadStarted || 'Upload started. Device will restart after update.');
  });

  document.getElementById('uploadFS').addEventListener('submit', async (e)=>{
    e.preventDefault();
    const fd = new FormData(e.target);
    const file = e.target.querySelector('input[name="fs"]').files[0];
    if (!file) return alert(TRANSLATIONS.alerts?.selectFile || 'Select file');
    const r = await fetch('/api/upload/fs', { method:'POST', body: fd });
    if (r.ok) alert(TRANSLATIONS.alerts?.fileUploaded || 'File uploaded');
  });

  // wifi form
  document.getElementById('wifiForm').addEventListener('submit', async (e)=>{
    e.preventDefault();
    const fd = new FormData(e.target);
    const ssid = fd.get('ssid'); const pass = fd.get('pass');
    const r = await fetch('/api/wifi', { method:'POST', body: new URLSearchParams({ssid, pass}) });
    const j = await r.json();
    document.getElementById('wifiResult').innerText = j.ok ? (TRANSLATIONS.wifi?.connected || 'Connected') : (TRANSLATIONS.wifi?.failed || 'Failed to connect');
  });

  // reboot form
  document.getElementById('rebootForm').addEventListener('submit', (e)=>{
    e.preventDefault();
    const fd = new FormData(e.target);
    const type = fd.get('type'); const delay = fd.get('delay');
    fetch('/api/reboot', { method:'POST', body: new URLSearchParams({type, delay}) });
    alert('Reboot scheduled');
  });

  // initial setup
  populateLanguageSelect();
  loadThemes();
  loadSystem();
  loadInfo();

  // preview + autosave when user changes selects (apply immediately, and persist to device)
  const langSel = document.getElementById('language');
  if (langSel) langSel.addEventListener('change', async (e)=>{
    const newLang = e.target.value;
    // optimistic apply
    await loadTranslations(newLang);
    // persist to device
    try{
      const r = await fetch('/api/setlanguage', { method:'POST', body: new URLSearchParams({lang:newLang}) });
      if (!r.ok) {
        console.warn('Failed to save language');
        // reload from device to revert
        loadSystem();
        alert(TRANSLATIONS.alerts?.saved === undefined ? 'Failed to save language' : TRANSLATIONS.alerts?.saved);
      }
    }catch(err){ console.warn('Error saving language', err); loadSystem(); }
  });

  const themeSel = document.getElementById('themeSelect');
  if (themeSel) themeSel.addEventListener('change', async (e)=>{
    const newTheme = e.target.value;
    const link = document.getElementById('theme');
    const prev = link ? link.href : null;
    if (link) link.href = `/themes/${newTheme}.css`;
    try{
      const r = await fetch('/api/settheme', { method:'POST', body: new URLSearchParams({theme:newTheme}) });
      if (!r.ok) {
        console.warn('Failed to save theme');
        // revert to device state
        loadSystem();
        alert(TRANSLATIONS.alerts?.themeSaved === undefined ? 'Failed to save theme' : TRANSLATIONS.alerts?.themeSaved);
        if (link && prev) link.href = prev;
      }
    }catch(err){ console.warn('Error saving theme', err); loadSystem(); if (link && prev) link.href = prev; }
  });
});