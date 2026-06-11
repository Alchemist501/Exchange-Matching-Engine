#pragma once

#include <string>

const std::string DASHBOARD_HTML = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>⚡ Exchange Dashboard - Low-Latency Engine</title>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;800&family=JetBrains+Mono:wght@400;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-base: #0b0f19;
            --bg-surface: #141c2f;
            --bg-card: #1d273f;
            --border: #2a3754;
            --text-primary: #f8fafc;
            --text-secondary: #94a3b8;
            --neon-buy: #10b981;
            --neon-sell: #ef4444;
            --neon-accent: #06b6d4;
            --glow-buy: rgba(16, 185, 129, 0.15);
            --glow-sell: rgba(239, 68, 68, 0.15);
            --glow-accent: rgba(6, 182, 212, 0.2);
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Outfit', sans-serif;
            background-color: var(--bg-base);
            color: var(--text-primary);
            overflow-x: hidden;
            display: flex;
            flex-direction: column;
            min-height: 100vh;
        }

        /* Top Bar */
        header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 1.2rem 2rem;
            background-color: var(--bg-surface);
            border-bottom: 1px solid var(--border);
            backdrop-filter: blur(8px);
        }

        .header-title-container {
            display: flex;
            align-items: center;
            gap: 0.8rem;
        }

        .logo-icon {
            font-size: 1.8rem;
            animation: pulse 2s infinite;
        }

        header h1 {
            font-size: 1.4rem;
            font-weight: 800;
            letter-spacing: -0.5px;
            background: linear-gradient(135deg, #38bdf8, #06b6d4, #00f5ff);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        .status-container {
            display: flex;
            align-items: center;
            gap: 0.6rem;
            background: rgba(255, 255, 255, 0.05);
            padding: 0.5rem 1rem;
            border-radius: 9999px;
            font-size: 0.85rem;
            font-weight: 600;
            border: 1px solid var(--border);
        }

        .status-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background-color: var(--neon-sell);
            box-shadow: 0 0 8px var(--neon-sell);
        }

        .status-dot.connected {
            background-color: var(--neon-buy);
            box-shadow: 0 0 10px var(--neon-buy);
        }

        /* Dashboard Main Layout */
        main {
            display: grid;
            grid-template-columns: 24rem 1fr 24rem;
            gap: 1.5rem;
            padding: 1.5rem;
            flex-grow: 1;
            height: calc(100vh - 80px);
            overflow: hidden;
        }

        @media (max-width: 1200px) {
            main {
                grid-template-columns: 1fr;
                height: auto;
                overflow: visible;
            }
        }

        /* Panels layout */
        .panel {
            background-color: var(--bg-surface);
            border: 1px solid var(--border);
            border-radius: 12px;
            display: flex;
            flex-direction: column;
            overflow: hidden;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.2);
        }

        .panel-header {
            padding: 1rem 1.2rem;
            border-bottom: 1px solid var(--border);
            font-weight: 600;
            font-size: 0.95rem;
            letter-spacing: 0.5px;
            text-transform: uppercase;
            color: var(--text-secondary);
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .panel-body {
            padding: 1.2rem;
            flex-grow: 1;
            overflow-y: auto;
        }

        /* Stats Section */
        .stats-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 1rem;
            margin-bottom: 1.2rem;
        }

        .stat-card {
            background-color: var(--bg-card);
            border: 1px solid var(--border);
            border-radius: 8px;
            padding: 0.8rem 1rem;
            display: flex;
            flex-direction: column;
            gap: 0.2rem;
        }

        .stat-label {
            font-size: 0.75rem;
            color: var(--text-secondary);
            text-transform: uppercase;
            font-weight: 600;
        }

        .stat-value {
            font-family: 'JetBrains Mono', monospace;
            font-size: 1.2rem;
            font-weight: 700;
        }

        .stat-value.glow-accent {
            color: var(--neon-accent);
            text-shadow: 0 0 8px var(--glow-accent);
        }

        /* Order Form */
        .order-form {
            display: flex;
            flex-direction: column;
            gap: 1rem;
        }

        .form-group {
            display: flex;
            flex-direction: column;
            gap: 0.4rem;
        }

        .form-group label {
            font-size: 0.8rem;
            color: var(--text-secondary);
            font-weight: 600;
        }

        .btn-toggle-container {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 0.5rem;
            margin-bottom: 0.5rem;
        }

        .btn-side {
            background-color: var(--bg-card);
            border: 1px solid var(--border);
            color: var(--text-secondary);
            padding: 0.6rem;
            border-radius: 6px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s ease;
        }

        .btn-side.active-buy {
            background-color: var(--neon-buy);
            color: white;
            border-color: var(--neon-buy);
            box-shadow: 0 0 12px var(--glow-buy);
        }

        .btn-side.active-sell {
            background-color: var(--neon-sell);
            color: white;
            border-color: var(--neon-sell);
            box-shadow: 0 0 12px var(--glow-sell);
        }

        .input-wrapper {
            position: relative;
        }

        .input-control {
            width: 100%;
            background-color: var(--bg-card);
            border: 1px solid var(--border);
            padding: 0.6rem 0.8rem;
            color: white;
            font-family: 'JetBrains Mono', monospace;
            border-radius: 6px;
            outline: none;
            transition: border-color 0.2s;
        }

        .input-control:focus {
            border-color: var(--neon-accent);
        }

        .input-hint {
            font-size: 0.7rem;
            color: var(--text-secondary);
            margin-top: 0.2rem;
        }

        .btn-submit {
            background: linear-gradient(135deg, #06b6d4, #0891b2);
            color: white;
            padding: 0.8rem;
            border-radius: 6px;
            border: none;
            font-weight: 600;
            cursor: pointer;
            transition: opacity 0.2s;
            margin-top: 0.5rem;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }

        .btn-submit:hover {
            opacity: 0.9;
        }

        /* Order Book Depth Column */
        .orderbook-ladder {
            display: flex;
            flex-direction: column;
            height: 100%;
        }

        .ladder-header {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            padding: 0.6rem 1rem;
            font-size: 0.75rem;
            font-weight: 700;
            color: var(--text-secondary);
            border-bottom: 1px solid var(--border);
        }

        .ladder-row {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            padding: 0.45rem 1rem;
            font-family: 'JetBrains Mono', monospace;
            font-size: 0.9rem;
            position: relative;
            align-items: center;
        }

        .price-col {
            font-weight: 700;
            z-index: 1;
        }

        .qty-col, .total-col {
            text-align: right;
            z-index: 1;
        }

        .price-col.buy { color: var(--neon-buy); }
        .price-col.sell { color: var(--neon-sell); }

        .fill-bar {
            position: absolute;
            top: 0.1rem;
            bottom: 0.1rem;
            right: 0;
            opacity: 0.12;
            transition: width 0.3s ease;
        }
        .fill-bar.buy { background-color: var(--neon-buy); }
        .fill-bar.sell { background-color: var(--neon-sell); }

        .spread-row {
            padding: 0.5rem 1rem;
            text-align: center;
            font-weight: 600;
            font-size: 0.8rem;
            color: var(--neon-accent);
            border-top: 1px dashed var(--border);
            border-bottom: 1px dashed var(--border);
            margin: 0.4rem 0;
            background: rgba(6, 182, 212, 0.03);
        }

        /* Trade Feed Column */
        .trade-feed-list {
            display: flex;
            flex-direction: column;
            gap: 0.5rem;
            height: 100%;
        }

        .trade-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 0.6rem 0.8rem;
            background-color: var(--bg-card);
            border-left: 3px solid var(--border);
            border-radius: 4px;
            font-family: 'JetBrains Mono', monospace;
            font-size: 0.85rem;
            animation: slideIn 0.25s ease-out;
        }

        .trade-row.buy { border-left-color: var(--neon-buy); }
        .trade-row.sell { border-left-color: var(--neon-sell); }

        .trade-left {
            display: flex;
            flex-direction: column;
            gap: 0.1rem;
        }

        .trade-time {
            font-size: 0.7rem;
            color: var(--text-secondary);
        }

        .trade-right {
            text-align: right;
        }

        .trade-price {
            font-weight: 700;
        }

        .trade-price.buy { color: var(--neon-buy); }
        .trade-price.sell { color: var(--neon-sell); }

        .trade-qty {
            font-size: 0.75rem;
            color: var(--text-secondary);
        }

        /* Utility animations */
        @keyframes pulse {
            0%, 100% { opacity: 1; transform: scale(1); }
            50% { opacity: 0.7; transform: scale(1.05); }
        }

        @keyframes slideIn {
            from { opacity: 0; transform: translateY(-5px); }
            to { opacity: 1; transform: translateY(0); }
        }

        /* Scrollbar customization */
        ::-webkit-scrollbar {
            width: 4px;
        }
        ::-webkit-scrollbar-track {
            background: rgba(0, 0, 0, 0.05);
        }
        ::-webkit-scrollbar-thumb {
            background: var(--border);
            border-radius: 2px;
        }
    </style>
</head>
<body>

    <header>
        <div class="header-title-container">
            <span class="logo-icon">⚡</span>
            <h1>Low-Latency Exchange Matching Engine</h1>
        </div>
        <div class="status-container">
            <div id="statusDot" class="status-dot"></div>
            <span id="statusText">Disconnected</span>
        </div>
    </header>

    <main>
        <!-- Left Panel: Stats & Order Form -->
        <div class="panel">
            <div class="panel-header">Market Operations</div>
            <div class="panel-body">
                <!-- Market Stats -->
                <div class="stats-grid">
                    <div class="stat-card">
                        <span class="stat-label">Best Bid</span>
                        <span id="statBid" class="stat-value">—</span>
                    </div>
                    <div class="stat-card">
                        <span class="stat-label">Best Ask</span>
                        <span id="statAsk" class="stat-value">—</span>
                    </div>
                    <div class="stat-card">
                        <span class="stat-label">Spread</span>
                        <span id="statSpread" class="stat-value">—</span>
                    </div>
                    <div class="stat-card">
                        <span class="stat-label">Mid Price</span>
                        <span id="statMid" class="stat-value">—</span>
                    </div>
                    <div class="stat-card">
                        <span class="stat-label">VWAP</span>
                        <span id="statVwap" class="stat-value glow-accent">—</span>
                    </div>
                    <div class="stat-card">
                        <span class="stat-label">Total Volume</span>
                        <span id="statVolume" class="stat-value">—</span>
                    </div>
                </div>

                <!-- Interactive Order Entry Form -->
                <div class="panel-header" style="padding-left:0; border-bottom:0; margin-top:1rem;">Order Gateway</div>
                <form id="orderForm" class="order-form" onsubmit="event.preventDefault(); submitOrder();">
                    <div class="form-group">
                        <label>Order Side</label>
                        <div class="btn-toggle-container">
                            <button type="button" id="btnBuy" class="btn-side active-buy" onclick="setSide('BUY')">BUY</button>
                            <button type="button" id="btnSell" class="btn-side" onclick="setSide('SELL')">SELL</button>
                        </div>
                    </div>
                    
                    <div class="form-group">
                        <label>Trader ID</label>
                        <input type="text" id="traderId" class="input-control" value="AlgoFundX" required>
                    </div>

                    <div class="form-group">
                        <label>Quantity</label>
                        <input type="number" id="quantity" class="input-control" value="100" min="1" required>
                    </div>

                    <div class="form-group">
                        <label>Price</label>
                        <input type="number" id="price" class="input-control" value="102.50" step="0.01" min="0.01">
                        <div class="input-hint">Leave blank or enter -1.0 for a Market Order</div>
                    </div>

                    <button type="submit" class="btn-submit">Submit Order</button>
                </form>
            </div>
        </div>

        <!-- Middle Panel: Live Order Book Depth -->
        <div class="panel">
            <div class="panel-header">Limit Order Book</div>
            <div class="panel-body" style="padding:0; overflow-y:hidden; display:flex; flex-direction:column;">
                <div class="orderbook-ladder">
                    <div class="ladder-header">
                        <span>PRICE</span>
                        <span style="text-align:right;">SIZE</span>
                        <span style="text-align:right;">TOTAL</span>
                    </div>
                    
                    <!-- Asks (Sells) - rendered top down (higher to lower) -->
                    <div id="asksContainer" style="display:flex; flex-direction:column-reverse; flex-grow:1; justify-content:flex-end; overflow-y:auto;">
                        <!-- JS injected -->
                    </div>

                    <!-- Bid-Ask Spread -->
                    <div class="spread-row" id="spreadRow">
                        Spread: —
                    </div>

                    <!-- Bids (Buys) - rendered top down (higher to lower) -->
                    <div id="bidsContainer" style="flex-grow:1; overflow-y:auto;">
                        <!-- JS injected -->
                    </div>
                </div>
            </div>
        </div>

        <!-- Right Panel: Live Matches Feed -->
        <div class="panel">
            <div class="panel-header">Live Execution Stream</div>
            <div class="panel-body" id="tradesContainer">
                <div class="trade-feed-list" id="tradesList">
                    <!-- Dynamic updates -->
                </div>
            </div>
        </div>
    </main>

    <script>
        let currentSide = 'BUY';
        let ws;

        function setSide(side) {
            currentSide = side;
            document.getElementById('btnBuy').className = side === 'BUY' ? 'btn-side active-buy' : 'btn-side';
            document.getElementById('btnSell').className = side === 'SELL' ? 'btn-side active-sell' : 'btn-side';
        }

        // Initialize WebSockets Connection
        function connectWebSocket() {
            const host = window.location.host;
            const wsUri = "ws://" + host + "/market";
            
            ws = new WebSocket(wsUri);
            
            ws.onopen = function() {
                document.getElementById('statusDot').className = 'status-dot connected';
                document.getElementById('statusText').innerText = 'Connected';
                document.getElementById('statusText').style.color = '#10b981';
            };
            
            ws.onclose = function() {
                document.getElementById('statusDot').className = 'status-dot';
                document.getElementById('statusText').innerText = 'Disconnected';
                document.getElementById('statusText').style.color = '#ef4444';
                // Try to reconnect in 3s
                setTimeout(connectWebSocket, 3000);
            };
            
            ws.onmessage = function(event) {
                const data = JSON.parse(event.data);
                if (data.event === 'depth') {
                    renderDepth(data.bids, data.asks);
                } else if (data.event === 'top') {
                    renderTop(data);
                } else if (data.event === 'trade') {
                    addTradeRecord(data);
                }
            };
        }

        function renderTop(data) {
            document.getElementById('statBid').innerText = data.bid > 0 ? data.bid.toFixed(2) : '—';
            document.getElementById('statAsk').innerText = data.ask > 0 ? data.ask.toFixed(2) : '—';
            document.getElementById('statSpread').innerText = data.spread > 0 ? data.spread.toFixed(2) : '—';
            document.getElementById('statMid').innerText = data.mid > 0 ? data.mid.toFixed(2) : '—';
            
            // Query stats for volume and VWAP via REST
            fetch('/market/stats')
                .then(res => res.json())
                .then(stats => {
                    document.getElementById('statVwap').innerText = stats.vwap > 0 ? stats.vwap.toFixed(4) : '—';
                    document.getElementById('statVolume').innerText = stats.volume.toLocaleString();
                });
        }

        function renderDepth(bids, asks) {
            // Asks
            const asksContainer = document.getElementById('asksContainer');
            asksContainer.innerHTML = '';
            
            let maxTotal = 1;
            let asksAccum = [];
            let totalAskQty = 0;
            asks.forEach(level => {
                totalAskQty += level[1];
                asksAccum.push({ price: level[0], size: level[1], total: totalAskQty });
            });
            maxTotal = Math.max(maxTotal, totalAskQty);

            // Bids
            const bidsContainer = document.getElementById('bidsContainer');
            bidsContainer.innerHTML = '';
            
            let bidsAccum = [];
            let totalBidQty = 0;
            bids.forEach(level => {
                totalBidQty += level[1];
                bidsAccum.push({ price: level[0], size: level[1], total: totalBidQty });
            });
            maxTotal = Math.max(maxTotal, totalBidQty);

            // Render Asks
            asksAccum.forEach(item => {
                const row = document.createElement('div');
                row.className = 'ladder-row';
                const pct = (item.total / maxTotal) * 100;
                row.innerHTML = `
                    <span class="price-col sell">${item.price.toFixed(2)}</span>
                    <span class="qty-col">${item.size.toLocaleString()}</span>
                    <span class="total-col">${item.total.toLocaleString()}</span>
                    <div class="fill-bar sell" style="width: ${pct}%"></div>
                `;
                asksContainer.appendChild(row);
            });

            // Render Bids
            bidsAccum.forEach(item => {
                const row = document.createElement('div');
                row.className = 'ladder-row';
                const pct = (item.total / maxTotal) * 100;
                row.innerHTML = `
                    <span class="price-col buy">${item.price.toFixed(2)}</span>
                    <span class="qty-col">${item.size.toLocaleString()}</span>
                    <span class="total-col">${item.total.toLocaleString()}</span>
                    <div class="fill-bar buy" style="width: ${pct}%"></div>
                `;
                bidsContainer.appendChild(row);
            });
        }

        function addTradeRecord(trade) {
            const list = document.getElementById('tradesList');
            const row = document.createElement('div');
            const isBuyTaker = trade.buyerId !== 'MarketMakerA' && trade.buyerId !== 'MarketMakerB';
            row.className = 'trade-row ' + (isBuyTaker ? 'buy' : 'sell');
            
            const timeStr = new Date(trade.timestamp / 1000000).toLocaleTimeString(); // nanosecond to ms

            row.innerHTML = `
                <div class="trade-left">
                    <span class="trade-price ${isBuyTaker ? 'buy' : 'sell'}">$${trade.price.toFixed(2)}</span>
                    <span class="trade-time">${timeStr}</span>
                </div>
                <div class="trade-right">
                    <span class="trade-qty">${trade.quantity.toLocaleString()} units</span>
                    <div style="font-size:0.65rem; color:var(--text-secondary);">${trade.buyerId} ➔ ${trade.sellerId}</div>
                </div>
            `;
            
            list.insertBefore(row, list.firstChild);
            if (list.children.length > 25) {
                list.removeChild(list.lastChild);
            }
        }

        function submitOrder() {
            const traderId = document.getElementById('traderId').value;
            const quantity = parseInt(document.getElementById('quantity').value);
            const priceVal = document.getElementById('price').value;
            const price = priceVal === '' ? -1.0 : parseFloat(priceVal);

            const payload = {
                traderId: traderId,
                side: currentSide,
                quantity: quantity,
                price: price
            };

            fetch('/orders', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            })
            .then(async res => {
                const data = await res.json();
                if (!res.ok) {
                    alert('Order Rejected: ' + (data.reason || 'Risk checks failed'));
                } else {
                    console.log('Order Accepted:', data);
                }
            })
            .catch(err => {
                alert('Connection error placing order');
            });
        }

        window.onload = function() {
            connectWebSocket();
        };
    </script>
</body>
</html>
)rawhtml";
