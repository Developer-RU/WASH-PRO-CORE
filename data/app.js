document.addEventListener('DOMContentLoaded', ()=>{
  const toggle = document.getElementById('toggle');
  const sidebar = document.getElementById('sidebar');
  let tasksUpdateInterval = null;
  let infoUpdateInterval = null;

  // Добавляем флаг для отслеживания фокуса на поле лицензии
  let licenseKeyHasFocus = false;
  let originalLicenseKeyValue = '';

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
      // Stop any running timers when navigating
      if (tasksUpdateInterval) { clearInterval(tasksUpdateInterval); tasksUpdateInterval = null; }
      if (infoUpdateInterval) { clearInterval(infoUpdateInterval); infoUpdateInterval = null; }

      document.querySelectorAll('.sidebar ul li').forEach(x=>x.classList.remove('active'));
      li.classList.add('active');
      document.querySelectorAll('.page').forEach(p=>p.classList.remove('active'));
      const page = document.getElementById(li.dataset.page);
      if (page) page.classList.add('active');
      document.getElementById('title').innerText = li.innerText;
      if (li.dataset.page === 'home') {
        loadInfo();
        infoUpdateInterval = setInterval(loadInfo, 5000); // Update every 5 seconds
      }
      if (li.dataset.page === 'tasks') {
        loadTasksEnhanced();
        tasksUpdateInterval = setInterval(loadTasksEnhanced, 5000); // Update every 5 seconds
      }
      if (li.dataset.page === 'system') loadSystem();
      if (li.dataset.page === 'files') loadFiles();
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
    document.querySelector('nav ul li[data-page="wifi"] span').textContent = t.nav?.wifi || 'Wi-Fi';
    document.querySelector('nav ul li[data-page="files"] span').textContent = t.nav?.files || 'File Manager';
    document.querySelector('nav ul li[data-page="update"] span').textContent = t.nav?.update || 'Update';
    document.querySelector('nav ul li[data-page="reboot"] span').textContent = t.nav?.reboot || 'Reboot';

    document.getElementById('infoTitle').textContent = t.info?.title || 'Information';
    document.getElementById('tasksTitle').textContent = t.tasks?.title || 'Tasks';
    document.getElementById('systemTitle').textContent = t.system?.title || 'System';
    // filesTitle is set dynamically in loadFiles()
    document.getElementById('updateTitle').textContent = t.update?.title || 'Update';
    document.getElementById('wifiTitle').textContent = t.wifi?.title || 'Wireless network';
    document.getElementById('langTitle').textContent = t.system?.language || 'Language';
    document.getElementById('licenseTitle').textContent = t.system?.licenseKey || 'License Key';
    document.getElementById('rebootTitle').textContent = t.reboot?.title || 'Reboot';
    document.getElementById('autoUpdateLabel').textContent = t.update?.autoUpdate || 'Automatic update';
    document.getElementById('themeTitle').textContent = t.system?.theme || 'Appearance';

    document.getElementById('licenseKey').placeholder = t.system?.licensePlaceholder || 'Enter license key...';
    document.getElementById('saveLicenseBtn').querySelector('.btn-text').textContent = t.system?.saveLicenseButton || 'Save Key';
    document.getElementById('createTaskBtn').querySelector('.btn-text').textContent = t.tasks?.create || 'Create Task';
    document.getElementById('createTaskBtn').title = t.tasks?.create || 'Create task';
    document.getElementById('uploadFirmwareBtn').querySelector('.btn-text').textContent = t.firmware?.uploadButton || 'Upload firmware (OTA)';
    document.getElementById('uploadFSBtn').querySelector('.btn-text').textContent = t.fs?.uploadButton || 'Upload to filesystem';
    document.getElementById('saveLang').querySelector('.btn-text').textContent = t.save?.button || 'Save';
    document.getElementById('saveTheme').querySelector('.btn-text').textContent = t.saveTheme?.button || 'Save theme';
    document.getElementById('wifiSaveBtn').querySelector('.btn-text').textContent = t.wifi?.saveButton || 'Save and connect';
    document.getElementById('rebootBtn').querySelector('.btn-text').textContent = t.reboot?.button || 'Reboot';
    document.getElementById('saveScriptBtn').querySelector('.btn-text').textContent = t.save?.button || 'Save';

    document.getElementById('closeEditor').textContent = t.tasks?.closeEditor || 'Close';
    const ssid = document.querySelector('#wifiForm input[name="ssid"]'); if (ssid) ssid.placeholder = t.wifi?.ssidPlaceholder || 'SSID';
    const ssidLabel = document.querySelector('#wifiForm label[for="ssidInput"]'); if (ssidLabel) ssidLabel.textContent = t.wifi?.ssidPlaceholder || 'SSID';
    const pass = document.querySelector('#wifiForm input[name="pass"]'); if (pass) pass.placeholder = t.wifi?.passPlaceholder || 'Password';
    const passLabel = document.querySelector('#wifiForm label[for="passInput"]'); if (passLabel) passLabel.textContent = t.wifi?.passPlaceholder || 'Password';
    const firmwareLabel = document.querySelector('#uploadFirmware label[for="firmwareFile"]'); if (firmwareLabel) firmwareLabel.textContent = t.update?.firmwareFile || 'Firmware file';
    const fsLabel = document.querySelector('#uploadFS label[for="fsFile"]'); if (fsLabel) fsLabel.textContent = t.update?.fsFile || 'Filesystem file';

    const rtype = document.getElementById('rebootTypeLabel'); if (rtype) rtype.textContent = t.reboot?.typeLabel || 'Type:';
    const rdelay = document.getElementById('rebootDelayLabel'); if (rdelay) rdelay.textContent = t.reboot?.delayLabel || 'Delay (sec):';
    const typeSel = document.querySelector('#rebootForm select[name="type"]');
    if (typeSel && typeSel.options.length > 1){ typeSel.options[0].textContent = t.reboot?.typeSoft || 'Soft'; typeSel.options[1].textContent = t.reboot?.typeHard || 'Hard'; }

    // If home page is active, reload its content with new translations
    if (document.getElementById('home').classList.contains('active')) {
      loadInfo(t.info).catch(e => console.error(e));
    }
    // Update the main header title with the translation for the currently active page
    const activeLi = document.querySelector('.sidebar ul li.active span');
    if (activeLi) document.getElementById('title').innerText = activeLi.innerText;

    if (document.getElementById('system').classList.contains('active')) {
      loadSystem().catch(e => console.error(e));
    }
    if (document.getElementById('files').classList.contains('active')) loadFiles().catch(e => console.error(e));
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

  async function loadInfo(translations){
    try {
      const [infoRes, tasksRes] = await Promise.all([fetch('/api/info'), fetch('/api/tasks')]);
      const j = await infoRes.json();
      const tasks = await tasksRes.json();
      const tasksArr = (tasks && tasks.tasks) ? tasks.tasks : (Array.isArray(tasks) ? tasks : []);
      const runningTasksCount = tasksArr.filter(t => t.state === 'running').length;
      const t = translations || TRANSLATIONS.info || {};
      document.getElementById('info').classList.add('loaded');
      document.getElementById('info').innerHTML = `<table>
        <tr><td>${t.serial || 'Serial'}</td><td>${j.serial || ''}</td></tr>
        <tr><td>${t.licenseActive || 'License active'}</td><td>${j.licenseActive ? (t.yes || 'Yes') : (t.no || 'No')}</td></tr>
        <tr><td>${t.freeHeap || 'Free heap'}</td><td>${j.freeHeap || ''}</td></tr>
        <tr><td>${t.heapSize || 'Heap size'}</td><td>${j.heapSize || ''}</td></tr>
        <tr><td>${t.createdTasks || 'Created tasks'}</td><td>${tasksArr.length}</td></tr>
        <tr><td>${t.runningTasks || 'Running tasks'}</td><td>${runningTasksCount}</td></tr>
      </table>`;
    } catch(e) { console.error("Failed to load info:", e); }
  }

  async function loadSystem(){
    return fetch('/api/system').then(r=>r.json()).then(async j=>{
      const lang = j.language || 'en';
      const theme = j.theme || 'gp_light';
      document.getElementById('language').value = lang;
      const themeLink = document.getElementById('theme'); if (themeLink) themeLink.href = `/themes/${theme}.css`;
      document.getElementById('autoUpdateCheckbox').checked = j.autoUpdate || false;
      
      // Сохраняем текущее значение поля, если пользователь его редактирует
      const licenseKeyField = document.getElementById('licenseKey');
      if (!licenseKeyHasFocus) {
        licenseKeyField.value = j.licenseKey || '';
        originalLicenseKeyValue = licenseKeyField.value;
      }
      
      const sel = document.getElementById('themeSelect'); if (sel) sel.value = theme;

      await loadTranslations(lang);
    });
  }

  let currentFileManagerPath = '/';

  function loadFiles(path = '/'){
    currentFileManagerPath = path; // Store current path
    fetch(`/api/files?path=${encodeURIComponent(currentFileManagerPath)}`).then(r=>r.json()).then(j=>{
      const container = document.getElementById('fileList');
      container.innerHTML = '';
      const currentPath = j.path || '/';
      let titleText = TRANSLATIONS.files?.title || 'File Manager';
      if (currentPath !== '/') {
        titleText += `: ${currentPath}`;
      }
      document.getElementById('filesTitle').textContent = titleText;

      const files = (j && j.files) ? j.files : [];
      if (files.length === 0 && currentPath === '/') {
        container.textContent = TRANSLATIONS.files?.noFiles || 'No files found.';
        return;
      }

      const TEXT_EXTENSIONS = ['txt', 'json', 'lua', 'js', 'css', 'html', 'md'];

      // Add ".." entry to go up
      if (currentPath !== '/') {
        const parentPath = currentPath.substring(0, currentPath.lastIndexOf('/')) || '/';
        const upCard = document.createElement('div');
        upCard.className = 'task-card'; // Use task-card style
        upCard.innerHTML = `<div class="task-info"><a href="#" class="nav-link" style="font-weight:bold;">..</a></div>`;
        upCard.querySelector('a').addEventListener('click', (e) => {
          e.preventDefault();
          loadFiles(parentPath);
        });
        container.appendChild(upCard);
      }

      files.forEach(file => {
        const card = document.createElement('div');
        card.className = 'task-card'; // Use task-card style
        const fullPath = (currentPath === '/' ? '' : currentPath) + '/' + file.name;
        
        const info = document.createElement('div');
        info.className = 'task-info';
        const title = document.createElement('div');
        title.style.fontWeight = '700';
        if (file.isDir) {
          const link = document.createElement('a');
          link.href = '#';
          link.className = 'nav-link';
          link.textContent = file.name;
          link.onclick = (e) => { e.preventDefault(); loadFiles(fullPath); };
          title.appendChild(link);
        } else {
          title.textContent = file.name;
        }
        const meta = document.createElement('div');
        meta.textContent = file.isDir ? 'Directory' : `${(file.size / 1024).toFixed(2)} KB`;
        info.appendChild(title);
        info.appendChild(meta);

        const actions = document.createElement('div');
        actions.className = 'task-actions';

        // Rename Button
        const renameBtn = document.createElement('button');
        renameBtn.className = 'file-action-btn';
        renameBtn.title = 'Rename';
        renameBtn.innerHTML = '<i class="fas fa-pencil"></i>';
        renameBtn.onclick = async () => {
          const newName = prompt('Enter new name for ' + file.name, file.name);
          if (newName && newName !== file.name) {
            await fetch('/api/files/rename', { method: 'POST', body: new URLSearchParams({ path: fullPath, newName: newName }) });
            loadFiles(currentPath);
          }
        };
        actions.appendChild(renameBtn);

        // Edit Button (for text files)
        const ext = file.name.split('.').pop().toLowerCase();
        if (!file.isDir && TEXT_EXTENSIONS.includes(ext)) {
          const editBtn = document.createElement('button');
          editBtn.className = 'file-action-btn';
          editBtn.title = 'Edit';
          editBtn.innerHTML = '<i class="fas fa-file-alt"></i>';
          editBtn.onclick = () => openFileEditor(fullPath);
          actions.appendChild(editBtn);
        }

        // Delete Button
        const deleteBtn = document.createElement('button');
        deleteBtn.className = 'file-action-btn';
        deleteBtn.title = TRANSLATIONS.files?.delete || 'Delete';
        deleteBtn.innerHTML = '<i class="fas fa-trash-can"></i>';
        deleteBtn.onclick = async () => {
          if (confirm(`${TRANSLATIONS.files?.deleteConfirm || 'Delete'} "${file.name}"?`)) {
            await fetch('/api/files/delete', { method: 'POST', body: new URLSearchParams({ path: fullPath }) });
            loadFiles(currentPath);
          }
        };
        actions.appendChild(deleteBtn);

        card.appendChild(info);
        card.appendChild(actions);
        container.appendChild(card);
      });
    });
  }

  let currentEditingFile = null;
  async function openFileEditor(filePath) {
    currentEditingTask = null;
    currentEditingFile = filePath;
    document.getElementById('editorTitle').textContent = `Edit: ${filePath}`;
    document.getElementById('scriptContent').value = 'Loading...';
    document.querySelector('.modal .builtins').style.display = 'none'; // Hide builtins for generic files
    
    const r = await fetch(filePath); // Directly fetch the file content
    if (r.ok) {
      const text = await r.text();
      document.getElementById('scriptContent').value = text;
    } else {
      document.getElementById('scriptContent').value = 'Error: Could not load file.';
    }
    document.getElementById('scriptEditor').style.display = 'flex';
  }

  // Enhanced tasks UI and script editor
  async function loadTasksEnhanced(){
    console.log('Loading tasks...');
    fetch('/api/tasks').then(r=>r.json()).then(j=>{
      const container = document.getElementById('tasksList');
      container.innerHTML = ''; // Always clear the container for a full redraw
      const arr = (j && j.tasks) ? j.tasks.sort((a,b) => a.name.localeCompare(b.name)) : (Array.isArray(j) ? j.sort((a,b) => a.name.localeCompare(b.name)) : []);
      
      if (arr.length === 0) {
        container.innerHTML = '<div class="empty-state">' + (TRANSLATIONS.tasks?.noTasks || 'No tasks found. Create your first task!') + '</div>';
        return;
      }
      
      arr.forEach(t=>{
        const card = document.createElement('div');
        card.className = 'task-card';

        const info = document.createElement('div'); 
        info.className = 'task-info';
        const title = document.createElement('div'); 
        title.textContent = t.name; 
        title.style.fontWeight='700';
        const meta = document.createElement('div');
        const stateStr = t.state || 'stopped';
        const translatedState = TRANSLATIONS.tasks?.status?.[stateStr] || stateStr;
        meta.textContent = `${TRANSLATIONS.tasks?.stateLabel || 'State'}: ${translatedState}${t.hasScript ? ` • ${TRANSLATIONS.tasks?.hasScript || 'has script'}` : ''}`;
        info.appendChild(title); 
        info.appendChild(meta);
        
        const actions = document.createElement('div'); 
        actions.className='task-actions';
        
        // helper to create an icon+label button
        function makeAction(icon, labelText, cb, btnClass = ''){
          const b = document.createElement('button');
          b.className = 'task-action-btn ' + btnClass;
          b.innerHTML = `<span class="btn-icon">${icon || ''}</span><span class="btn-text">${labelText}</span>`;
          b.title = labelText;
          b.setAttribute('aria-label', labelText);
          b.addEventListener('click', cb);
          return b;
        }

        // 1. Edit Task button (rename)
        const renameLabel = TRANSLATIONS.tasks?.renamePrompt || 'Rename task';
        const taskId = String(t.id);
        const editTaskBtn = makeAction('<i class="fas fa-pencil"></i>', renameLabel, async ()=>{
          const newName = prompt(renameLabel, t.name);
          if (!newName || newName === t.name) return;
          const r = await fetch('/api/tasks', { method:'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: new URLSearchParams({ name: newName, id: taskId }) });
          if (r.ok) loadTasksEnhanced();
          else { const err = await r.text(); console.error('Rename failed:', r.status, err); alert('Rename failed: ' + (err || r.status)); }
        });

        // 2. Edit Script button
        const scriptLabel = t.hasScript? (TRANSLATIONS.tasks?.editScript||'Edit script') : (TRANSLATIONS.tasks?.attachScript||'Attach script');
        const scriptBtn = makeAction('<i class="fas fa-file-alt"></i>', scriptLabel, ()=>{ openScriptEditor(taskId, t.name); });

        // 3. Delete button (deletes both task and script)
        const delLabel = TRANSLATIONS.tasks?.delete || 'Delete';
        const delBtn = makeAction('<i class="fas fa-trash-can"></i>', delLabel, async ()=>{
          if (!confirm(`${TRANSLATIONS.tasks?.deleteConfirm || 'Delete task'} "${t.name}"?`)) return;

          // Stop updates during delete
          if (tasksUpdateInterval) { clearInterval(tasksUpdateInterval); tasksUpdateInterval = null; }

          const taskFilePath = `/tasks/${taskId}.json`;
          const scriptFilePath = `/scripts/${taskId}.lua`;

          const deletePromises = [];
          deletePromises.push(
            fetch('/api/files/delete', { method: 'POST', body: new URLSearchParams({ path: taskFilePath }) })
          );

          if (t.hasScript) {
            deletePromises.push(
              fetch('/api/files/delete', { method: 'POST', body: new URLSearchParams({ path: scriptFilePath }) })
            );
          }

          const results = await Promise.all(deletePromises);
          const allOk = results.every(r => r.ok);

          if (allOk) {
              loadTasksEnhanced(); // Reload immediately after successful delete
          } else {
              console.error('Delete failed:', results); 
              alert('Delete failed'); 
          }
          
          // Restart updates, but only if the page is still active
          if (document.getElementById('tasks').classList.contains('active')) {
              tasksUpdateInterval = setInterval(loadTasksEnhanced, 5000);
          }
        });

        // 4. Run/Stop button - ВАЖНО: исправляем логику отображения кнопок
        if (t.state === 'running') {
          // Если задача запущена, показываем кнопку остановки
          const stopLabel = TRANSLATIONS.tasks?.stop || 'Stop';
          const stopBtn = makeAction('<i class="fas fa-stop"></i>', stopLabel, async () => { 
            try {
              console.log('Stopping task:', taskId);
              const response = await fetch('/api/tasks/stop', { 
                method: 'POST', 
                headers: {'Content-Type': 'application/x-www-form-urlencoded'}, 
                body: new URLSearchParams({ id: taskId }) 
              });
              
              if (response.ok) {
                // Оптимистичное обновление UI
                meta.textContent = `${TRANSLATIONS.tasks?.stateLabel || 'State'}: ${TRANSLATIONS.tasks?.status?.stopped || 'stopped'}${t.hasScript ? ` • ${TRANSLATIONS.tasks?.hasScript || 'has script'}` : ''}`;
                alert(TRANSLATIONS.tasks?.stopSuccess || 'Task stopped');
                
                // Обновляем список через 1 секунду для подтверждения
                setTimeout(() => {
                  if (document.getElementById('tasks').classList.contains('active')) {
                    loadTasksEnhanced();
                  }
                }, 1000);
              } else {
                const errorText = await response.text();
                alert('Failed to stop task: ' + errorText);
              }
            } catch (error) {
              console.error('Error stopping task:', error);
              alert('Error stopping task: ' + error.message);
            }
          }, 'stop-btn');
          actions.appendChild(stopBtn);
        } else {
          // Если задача остановлена, показываем кнопку запуска
          const runLabel = TRANSLATIONS.tasks?.run || 'Run';
          const runBtn = makeAction('<i class="fas fa-play"></i>', runLabel, async () => { 
            try {
              console.log('Running task:', taskId);
              const response = await fetch('/api/tasks/run', { 
                method: 'POST', 
                headers: {'Content-Type': 'application/x-www-form-urlencoded'}, 
                body: new URLSearchParams({ id: taskId }) 
              });
              
              console.log('Response status:', response.status);
              const result = await response.text();
              console.log('Response text:', result);
              
              if (response.ok) {
                try {
                  const json = JSON.parse(result);
                  alert(json.message || (TRANSLATIONS.tasks?.runStarted || 'Task started successfully'));
                  
                  // Оптимистичное обновление UI
                  meta.textContent = `${TRANSLATIONS.tasks?.stateLabel || 'State'}: ${TRANSLATIONS.tasks?.status?.running || 'running'}${t.hasScript ? ` • ${TRANSLATIONS.tasks?.hasScript || 'has script'}` : ''}`;
                  
                  // Обновляем список через 1 секунду для подтверждения
                  setTimeout(() => {
                    if (document.getElementById('tasks').classList.contains('active')) {
                      loadTasksEnhanced();
                    }
                  }, 1000);
                } catch (e) {
                  console.error('Error parsing response:', e);
                  alert(TRANSLATIONS.tasks?.runStarted || 'Task started');
                  // Все равно обновляем список
                  setTimeout(() => {
                    if (document.getElementById('tasks').classList.contains('active')) {
                      loadTasksEnhanced();
                    }
                  }, 1000);
                }
              } else {
                try {
                  const json = JSON.parse(result);
                  alert('Failed to start task: ' + (json.error || 'Unknown error'));
                } catch (e) {
                  alert('Failed to start task: ' + result);
                }
              }
            } catch (error) {
              console.error('Error running task:', error);
              alert('Failed to start task: Network error');
            }
          }, 'run-btn');
          actions.appendChild(runBtn);
        }

        actions.appendChild(editTaskBtn); 
        actions.appendChild(scriptBtn); 
        actions.appendChild(delBtn);
        card.appendChild(info); 
        card.appendChild(actions);
        container.appendChild(card);
      });
    }).catch(error => {
      console.error('Failed to load tasks:', error);
      const container = document.getElementById('tasksList');
      container.innerHTML = '<div class="error-state">' + (TRANSLATIONS.tasks?.loadError || 'Failed to load tasks. Please try again.') + '</div>';
    });
  }
  
  // Create task button
  document.getElementById('createTaskBtn')?.addEventListener('click', async ()=>{
    const name = prompt(TRANSLATIONS.tasks?.createPrompt || 'Task name'); 
    if (!name) return;
    
    try {
      const r = await fetch('/api/tasks', { 
        method:'POST', 
        headers: {'Content-Type': 'application/x-www-form-urlencoded'}, 
        body: new URLSearchParams({ name }) 
      });
      
      if (r.ok) {
        loadTasksEnhanced();
      } else { 
        const err = await r.text(); 
        console.error('Create task failed:', r.status, err); 
        alert('Create failed: ' + (err || r.status)); 
      }
    } catch (error) {
      console.error('Error creating task:', error);
      alert('Error creating task: ' + error.message);
    }
  });

  // Script editor utilities
  let currentEditingTask = null;

  async function openScriptEditor(taskId, taskName) {
    const id = String(taskId);
    currentEditingTask = id;
    currentEditingFile = null;
    document.getElementById('editorTitle').textContent = `${TRANSLATIONS.tasks?.scriptFor || 'Script:'} ${taskName || id}`;
    document.getElementById('scriptContent').value = 'Loading...';
    document.querySelector('.modal .builtins').style.display = 'block';
    const scriptEl = document.getElementById('scriptContent');

    // CONSTRUCT THE PATH, as per the user's instruction to mimic the file editor.
    const scriptPath = `/scripts/${id}.lua`;

    try {
        // Fetch the script content directly from its path.
        const rScript = await fetch(scriptPath);
        if (rScript.ok) {
            scriptEl.value = await rScript.text();
        } else {
            // If the script doesn't exist (e.g., new task), the fetch will fail (404),
            // and the editor should be empty. This is correct.
            scriptEl.value = '';
        }
    } catch (e) {
        console.error(`Failed to fetch script from ${scriptPath}:`, e);
        scriptEl.value = `Error loading script.`;
    }
    
    // Optionally refresh task name from task API (for title only). This is still useful.
    try {
      const rTask = await fetch(`/api/tasks/${encodeURIComponent(id)}`);
      if (rTask.ok) {
          const task = await rTask.json();
          if (task && task.name) {
              document.getElementById('editorTitle').textContent = `${TRANSLATIONS.tasks?.scriptFor || 'Script:'} ${task.name}`;
          }
      }
    } catch (e) {
      console.error('Error fetching task details:', e);
    }

    // load builtins
    fetch('/api/builtins').then(r => r.json()).then(list => {
        const ul = document.getElementById('builtinList');
        ul.innerHTML = '';
        list.forEach(fn => {
            const li = document.createElement('li');
            li.textContent = fn;
            li.addEventListener('click', () => { insertAtCursor(scriptEl, fn + '()'); });
            ul.appendChild(li);
        });
    }).catch(e => {
      console.error('Failed to load builtins:', e);
    });
    
    document.getElementById('scriptEditor').style.display = 'flex';
  }
  
  document.getElementById('closeEditor')?.addEventListener('click', ()=>{ 
    document.getElementById('scriptEditor').style.display = 'none'; 
    currentEditingTask = null; 
    currentEditingFile = null; 
  });
  
  document.getElementById('saveScriptBtn')?.addEventListener('click', async ()=>{
    const content = document.getElementById('scriptContent').value;

    if (currentEditingTask) { // Saving a task script
      const taskName = document.getElementById('editorTitle').textContent.replace(`${TRANSLATIONS.tasks?.scriptFor || 'Script:'} `, '');
      const payload = new URLSearchParams({ id: currentEditingTask, name: taskName, script: content });
      const r = await fetch('/api/tasks', { method:'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: payload });
      if (r.ok){ 
        alert(TRANSLATIONS.alerts?.saved || 'Saved'); 
        document.getElementById('scriptEditor').style.display='none'; 
        loadTasksEnhanced(); 
      } else { 
        const txt = await r.text(); 
        console.error('Save script failed:', r.status, txt); 
        alert('Failed to save: ' + txt); 
      }
    } else if (currentEditingFile) { // Saving a generic file
      const payload = new URLSearchParams({ path: currentEditingFile, content: content });
      const r = await fetch('/api/files/save', { method:'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: payload });
      if (r.ok){ 
        alert(TRANSLATIONS.alerts?.saved || 'Saved'); 
        document.getElementById('scriptEditor').style.display='none'; 
        loadFiles(currentFileManagerPath); 
      } else { 
        const txt = await r.text(); 
        console.error('Save file failed:', r.status, txt); 
        alert('Failed to save: ' + txt); 
      }
    }
  });

  function insertAtCursor(el, text){
    const start = el.selectionStart || 0; 
    const end = el.selectionEnd || 0; 
    const v = el.value;
    el.value = v.slice(0,start) + text + v.slice(end);
    el.selectionStart = el.selectionEnd = start + text.length;
    el.focus();
  }

  document.getElementById('saveLang').addEventListener('click', async ()=>{
    const lang = document.getElementById('language').value;
    const r = await fetch('/api/setlanguage', { method:'POST', body: new URLSearchParams({lang}) });
    if (r.ok){ 
      await loadTranslations(lang); 
      alert(TRANSLATIONS.alerts?.saved || 'Saved'); 
    } else {
      alert('Failed to save language');
    }
  });

  // license save
  document.getElementById('saveLicenseBtn')?.addEventListener('click', async ()=>{
    const key = document.getElementById('licenseKey').value;
    const r = await fetch('/api/setlicense', { method:'POST', body: new URLSearchParams({key}) });
    if (r.ok) {
      alert(TRANSLATIONS.alerts?.saved || 'Saved');
      originalLicenseKeyValue = key;
    } else {
      alert('Failed to save license key');
    }
  });

  // theme save
  document.getElementById('saveTheme')?.addEventListener('click', async ()=>{
    const theme = document.getElementById('themeSelect').value;
    const r = await fetch('/api/settheme', { method:'POST', body: new URLSearchParams({theme}) });
    if (r.ok){ 
      document.getElementById('theme').href = `/themes/${theme}.css`; 
      alert(TRANSLATIONS.alerts?.themeSaved || 'Theme saved'); 
    } else {
      alert('Failed to save theme');
    }
  });

  // uploads
  document.getElementById('uploadFirmware').addEventListener('submit', async (e)=>{
    e.preventDefault();
    const fd = new FormData(e.target);
    const file = e.target.querySelector('input[name="firmware"]').files[0];
    if (!file) return alert(TRANSLATIONS.alerts?.selectFile || 'Select file');
    const r = await fetch('/api/upload/firmware', { method:'POST', body: fd });
    if (r.ok) alert(TRANSLATIONS.alerts?.uploadStarted || 'Upload started. Device will restart after update.');
    else alert('Upload failed');
  });

  document.getElementById('uploadFS').addEventListener('submit', async (e)=>{
    e.preventDefault();
    const fd = new FormData(e.target);
    const file = e.target.querySelector('input[name="fs"]').files[0];
    if (!file) return alert(TRANSLATIONS.alerts?.selectFile || 'Select file');
    const r = await fetch('/api/upload/fs', { method:'POST', body: fd });
    if (r.ok) alert(TRANSLATIONS.alerts?.fileUploaded || 'File uploaded');
    else alert('Upload failed');
  });

  // wifi form
  document.getElementById('wifiForm').addEventListener('submit', async (e)=>{
    e.preventDefault();
    const fd = new FormData(e.target);
    const ssid = fd.get('ssid'); 
    const pass = fd.get('pass');
    const r = await fetch('/api/wifi', { method:'POST', body: new URLSearchParams({ssid, pass}) });
    const j = await r.json();
    document.getElementById('wifiResult').innerText = j.ok ? (TRANSLATIONS.wifi?.connected || 'Connected') : (TRANSLATIONS.wifi?.failed || 'Failed to connect');
  });

  // reboot form
  document.getElementById('rebootForm').addEventListener('submit', (e)=>{
    e.preventDefault();
    const fd = new FormData(e.target);
    const type = fd.get('type'); 
    const delay = fd.get('delay');
    fetch('/api/reboot', { method:'POST', body: new URLSearchParams({type, delay}) });
    alert('Reboot scheduled');
  });

  // initial setup
  populateLanguageSelect();
  loadThemes();
  loadSystem();
  loadInfo();

  // Добавляем обработчики фокуса для поля лицензионного ключа
  const licenseKeyField = document.getElementById('licenseKey');
  if (licenseKeyField) {
    licenseKeyField.addEventListener('focus', () => {
      licenseKeyHasFocus = true;
      originalLicenseKeyValue = licenseKeyField.value;
    });
    
    licenseKeyField.addEventListener('blur', () => {
      licenseKeyHasFocus = false;
    });
  }

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
        // reload from device to revert - но не обновляем поле лицензии если пользователь его редактирует
        const tempLicenseKey = document.getElementById('licenseKey').value;
        await loadSystem();
        // Восстанавливаем значение поля лицензии, если пользователь его редактировал
        if (licenseKeyHasFocus) {
          document.getElementById('licenseKey').value = tempLicenseKey;
        }
        alert(TRANSLATIONS.alerts?.saved === undefined ? 'Failed to save language' : TRANSLATIONS.alerts?.saved);
      }
    }catch(err){ 
      console.warn('Error saving language', err); 
      const tempLicenseKey = document.getElementById('licenseKey').value;
      await loadSystem();
      if (licenseKeyHasFocus) {
        document.getElementById('licenseKey').value = tempLicenseKey;
      }
    }
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
        // revert to device state - но не обновляем поле лицензии если пользователь его редактирует
        const tempLicenseKey = document.getElementById('licenseKey').value;
        await loadSystem();
        // Восстанавливаем значение поля лицензии, если пользователь его редактировал
        if (licenseKeyHasFocus) {
          document.getElementById('licenseKey').value = tempLicenseKey;
        }
        alert(TRANSLATIONS.alerts?.themeSaved === undefined ? 'Failed to save theme' : TRANSLATIONS.alerts?.themeSaved);
        if (link && prev) link.href = prev;
      }
    }catch(err){ 
      console.warn('Error saving theme', err); 
      const tempLicenseKey = document.getElementById('licenseKey').value;
      await loadSystem();
      if (licenseKeyHasFocus) {
        document.getElementById('licenseKey').value = tempLicenseKey;
      }
      if (link && prev) link.href = prev; 
    }
  });
});

document.getElementById('autoUpdateCheckbox')?.addEventListener('change', async (e) => {
  const enabled = e.target.checked;
  await fetch('/api/autoupdate', { method: 'POST', body: new URLSearchParams({ enabled: enabled }) });
});

// SSE for real-time updates
const eventSource = new EventSource('/events');

eventSource.addEventListener('tasks_update', function(e) {
  console.log('Task update received:', e.data);
  // If tasks page is active, reload tasks
  if (document.getElementById('tasks').classList.contains('active')) {
    console.log('Refreshing tasks list due to SSE event');
    loadTasksEnhanced();
  }
  // If home page is active, reload info
  if (document.getElementById('home').classList.contains('active')) {
    loadInfo();
  }
});

eventSource.addEventListener('keepalive', function(e) {
  console.log('SSE keepalive received');
});

eventSource.addEventListener('open', function(e) {
  console.log('SSE connection opened');
});

eventSource.onerror = function(e) {
  console.error('EventSource failed:', e);
  // Try to reconnect after 5 seconds
  setTimeout(() => {
    console.log('Reconnecting SSE...');
    eventSource.close();
    // Create new connection
    const newEventSource = new EventSource('/events');
    newEventSource.addEventListener('tasks_update', function(e) {
      if (document.getElementById('tasks').classList.contains('active')) {
        loadTasksEnhanced();
      }
      if (document.getElementById('home').classList.contains('active')) {
        loadInfo();
      }
    });
  }, 5000);
};

// Clean up on page unload
window.addEventListener('beforeunload', function() {
  eventSource.close();
  if (tasksUpdateInterval) clearInterval(tasksUpdateInterval);
  if (infoUpdateInterval) clearInterval(infoUpdateInterval);
});