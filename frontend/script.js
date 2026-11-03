const overlay    = document.getElementById('loginOverlay');
const loginForm  = document.getElementById('loginForm');
const resultArea = document.getElementById('resultArea');
const resultBody = document.getElementById('resultBody');
const loginMsg   = document.getElementById('loginMsg');

let ballotsById = {};     // id -> ballot object, filled by loadBallots
let pending = null;       // what the user clicked: {type:'open', ballot} or {type:'create'}

/* ---------- show / hide the overlay ---------- */
function openLogin(action, title) {
    pending = action;
    document.getElementById('loginTitle').textContent = title;
    loginMsg.textContent = '';
    document.getElementById('password').value = '';
    loginForm.classList.remove('hidden');
    resultArea.classList.add('hidden');
    overlay.classList.remove('hidden');          // show
}

function closeLogin() {
    overlay.classList.add('hidden');             // hide
    pending = null;
}

/* ---------- ballot table ---------- */
async function loadBallots() {
    const r = await fetch('/api/ballots');
    const list = await r.json();

    const table = document.createElement('table');
    table.className = 'table table-hover mb-0';
    table.innerHTML = '<thead><tr><th>Title</th><th>Admin</th><th>Status</th></tr></thead>';  // fixed text only
    const body = document.createElement('tbody');

    ballotsById = {};
    for (const b of list) {
        ballotsById[b.id] = b;
        const tr = document.createElement('tr');
        tr.dataset.id = b.id;
        tr.style.cursor = 'pointer';

        const t = document.createElement('td');
        t.textContent = b.title;                 // textContent: safe for user text
        const a = document.createElement('td');
        a.textContent = b.admin;

        const st = document.createElement('td');
        const badge = document.createElement('span');
        badge.className = 'badge ' + (b.ended ? 'bg-secondary' : 'bg-success');
        badge.textContent = b.ended ? 'Ended' : 'Open';
        st.appendChild(badge);

        tr.append(t, a, st);
        body.appendChild(tr);
    }
    table.appendChild(body);

    const box = document.getElementById('ballots');
    box.textContent = '';
    box.appendChild(table);
}

// one listener on the container (event delegation)
document.getElementById('ballots').addEventListener('click', (e) => {
    const row = e.target.closest('tr[data-id]');
    if (!row) return;                            // clicked the header or empty space
    const ballot = ballotsById[row.dataset.id];
    openLogin({ type: 'open', ballot }, ballot.title);
});

/* ---------- create buttons ---------- */
function startCreate(e) {
    e.preventDefault();
    openLogin({ type: 'create' }, 'Login to create a ballot');
}
document.getElementById('createBtn').addEventListener('click', startCreate);
document.getElementById('adminBtn').addEventListener('click', () => openLogin({ type: 'super' }, 'Admin login'));

/* ---------- cancel / close ---------- */
document.getElementById('cancelBtn').addEventListener('click', closeLogin);
document.getElementById('closeBtn').addEventListener('click', closeLogin);

/* ---------- submit ---------- */
loginForm.addEventListener('submit', async (e) => {
    e.preventDefault();                          // no page reload
    const roll = document.getElementById('roll').value;
    const password = document.getElementById('password').value;

    const url = pending.type === 'open' ? '/api/ballots/' + pending.ballot.id + '/access'
              : pending.type === 'super' ? '/api/super/login'
              : '/api/login';

    const r = await fetch(url, { method: 'POST', body: new URLSearchParams({ roll, password }) });
    const data = await r.json();

    if (!data.ok) { loginMsg.textContent = data.message; return; }

    if (pending.type === 'super') {
        sessionStorage.setItem('roll', roll);
        sessionStorage.setItem('password', password);
        window.location.href = 'superadmin.html';
    } else if (pending.type === 'create') {
        sessionStorage.setItem('roll', roll);
        sessionStorage.setItem('password', password);
        window.location.href = 'create.html';
    } else if (data.state === 'ended') {
        sessionStorage.removeItem('password');
        sessionStorage.setItem('results', JSON.stringify(data));
        window.location.href = 'results.html';
    } else if (data.state === 'admin') {
        sessionStorage.setItem('ballotId', pending.ballot.id);
        sessionStorage.setItem('roll', roll);
        sessionStorage.setItem('password', password);
        window.location.href = 'admin.html';
    } else if (data.state === 'eligible') {
        sessionStorage.setItem('ballotId', pending.ballot.id);
        sessionStorage.setItem('roll', roll);
        sessionStorage.setItem('password', password);
        window.location.href = 'vote.html';
    } else if (data.state === 'voted') {
        sessionStorage.setItem('votedFor', data.candidate + ' - ' + data.name);
        sessionStorage.setItem('ballotTitle', pending.ballot.title);
        window.location.href = 'already_voted.html';
    }
});

loadBallots();