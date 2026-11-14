// Sound output for the web port: the game's audio interface (port/sdk/ai.c)
// hands every finished 32 kHz stereo DMA block to Module.audioPush; this feeds
// them to WebAudio through a small queue (AudioWorklet, or a
// ScriptProcessorNode where worklets are unavailable). The AudioContext starts
// on the Start button click, since browsers only allow audio after a gesture.
(() => {
  const RATE = 32000;
  const PREBUFFER = 0.06; // seconds queued before playback (re)starts
  const MAX_QUEUE = 0.25; // seconds; older blocks are dropped beyond this

  // Shared by the worklet and the ScriptProcessor fallback: a queue of
  // interleaved stereo Float32 blocks read with linear interpolation at
  // `step` source frames per output frame.
  const queueSource = `
    class Queue {
      constructor(step, prebuffer, maxQueue) {
        this.step = step; this.prebuffer = prebuffer; this.maxQueue = maxQueue;
        this.blocks = []; this.frames = 0; this.pos = 0; this.primed = false;
        this.prevL = 0; this.prevR = 0;
      }
      push(block) {
        this.blocks.push(block); this.frames += block.length / 2;
        while (this.frames > this.maxQueue && this.blocks.length > 1) {
          const b = this.blocks.shift(); this.frames -= b.length / 2 - this.pos; this.pos = 0;
        }
      }
      sample(i) { // frame at offset i past the read position, or null
        let p = this.pos + i;
        for (const b of this.blocks) { const n = b.length / 2; if (p < n) return b.subarray(p * 2, p * 2 + 2); p -= n; }
        return null;
      }
      render(outL, outR) {
        if (!this.primed && this.frames >= this.prebuffer) this.primed = true;
        for (let i = 0; i < outL.length; i++) {
          if (!this.primed) { outL[i] = outR[i] = 0; continue; }
          const a = this.sample(0), b = this.sample(1);
          if (!a || !b) { this.primed = false; outL[i] = outR[i] = 0; continue; }
          const t = this.frac || 0;
          outL[i] = a[0] + (b[0] - a[0]) * t;
          outR[i] = a[1] + (b[1] - a[1]) * t;
          this.frac = t + this.step;
          while (this.frac >= 1) {
            this.frac -= 1; this.pos++; this.frames--;
            if (this.pos >= this.blocks[0].length / 2) { this.blocks.shift(); this.pos = 0; }
          }
        }
      }
    }`;

  let ctx = null;
  let send = null;

  Module.audioPush = (lr) => {
    if (!send) return;
    const f = new Float32Array(lr.length);
    for (let i = 0; i < lr.length; i++) f[i] = lr[i] / 32768;
    send(f);
  };

  async function start() {
    if (ctx) return;
    const AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return;
    try { ctx = new AC({ sampleRate: RATE }); } catch (e) { ctx = new AC(); }
    Module.audioContext = ctx;
    const step = RATE / ctx.sampleRate;
    const args = [step, RATE * PREBUFFER, RATE * MAX_QUEUE];
    if (ctx.audioWorklet) {
      try {
        const src = queueSource + `
          registerProcessor('mkdd-audio', class extends AudioWorkletProcessor {
            constructor() { super(); this.q = new Queue(${args.join(',')}); this.port.onmessage = (e) => this.q.push(e.data); }
            process(inputs, outputs) { const o = outputs[0]; this.q.render(o[0], o[1] || o[0]); return true; }
          });`;
        const url = URL.createObjectURL(new Blob([src], { type: 'application/javascript' }));
        await ctx.audioWorklet.addModule(url);
        const node = new AudioWorkletNode(ctx, 'mkdd-audio', { numberOfInputs: 0, outputChannelCount: [2] });
        node.connect(ctx.destination);
        send = (f) => node.port.postMessage(f, [f.buffer]);
      } catch (e) {
        console.warn('AudioWorklet unavailable, using ScriptProcessorNode', e);
      }
    }
    if (!send) {
      const Queue = new Function(queueSource + '; return Queue;')();
      const q = new Queue(...args);
      const sp = ctx.createScriptProcessor(1024, 0, 2);
      sp.onaudioprocess = (e) => q.render(e.outputBuffer.getChannelData(0), e.outputBuffer.getChannelData(1));
      sp.connect(ctx.destination);
      send = (f) => q.push(f);
    }
    if (ctx.state !== 'running') await ctx.resume();
  }

  const btn = document.getElementById('start');
  if (btn) btn.addEventListener('click', () => { start().catch((e) => console.warn('audio start failed', e)); });
  // browsers may suspend the context (tab switch); a later gesture resumes it
  addEventListener('pointerdown', () => { if (ctx && ctx.state === 'suspended') ctx.resume(); });
  addEventListener('keydown', () => { if (ctx && ctx.state === 'suspended') ctx.resume(); });
})();
