// ==============================================================================
// SP Programming Language - Cloudflare R2 Release Uploader (Node.js Engine)
// Zero external dependencies - pure Node.js standard library (crypto, fs, fetch)
// ==============================================================================

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

function loadEnv(envPath) {
  if (!fs.existsSync(envPath)) return {};
  const content = fs.readFileSync(envPath, 'utf8');
  const env = {};
  for (const line of content.split(/\r?\n/)) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith('#')) continue;
    const eqIdx = trimmed.indexOf('=');
    if (eqIdx !== -1) {
      const key = trimmed.slice(0, eqIdx).trim();
      const val = trimmed.slice(eqIdx + 1).trim().replace(/^["']|["']$/g, '');
      env[key] = val;
    }
  }
  return env;
}

const repoRoot = path.resolve(__dirname, '..');
const envVars = { ...process.env, ...loadEnv(path.join(repoRoot, '.env')) };

const ACCOUNT_ID = envVars.R2_ACCOUNT_ID || '3b56f1445438ff6041fee436fe29b097';
const BUCKET_NAME = envVars.R2_BUCKET_NAME || 'splang-releases';
const ACCESS_KEY_ID = envVars.R2_ACCESS_KEY_ID;
const SECRET_ACCESS_KEY = envVars.R2_SECRET_ACCESS_KEY;

if (!ACCESS_KEY_ID || !SECRET_ACCESS_KEY) {
  console.error('[ERROR] R2_ACCESS_KEY_ID or R2_SECRET_ACCESS_KEY not found in .env or environment.');
  process.exit(1);
}

function hmac(key, data) {
  return crypto.createHmac('sha256', key).update(data).digest();
}

function hash(data) {
  return crypto.createHash('sha256').update(data).digest('hex');
}

async function uploadFileToR2(filePath, objectKey, contentType = 'application/octet-stream') {
  if (!fs.existsSync(filePath)) {
    throw new Error(`File not found: ${filePath}`);
  }

  const fileBuffer = fs.readFileSync(filePath);
  const fileBytes = fileBuffer.length;
  const payloadHash = hash(fileBuffer);
  
  const host = `${ACCOUNT_ID}.r2.cloudflarestorage.com`;
  const canonicalUri = `/${BUCKET_NAME}/${objectKey}`;
  const endpoint = `https://${host}${canonicalUri}`;

  const now = new Date();
  const amzDate = now.toISOString().replace(/[:-]|\.\d{3}/g, '');
  const dateStamp = amzDate.substring(0, 8);
  const region = 'auto';
  const service = 's3';

  // Headers must be sorted by lowercase name
  const headers = {
    'cache-control': 'public, max-age=300, must-revalidate',
    'content-type': contentType,
    'host': host,
    'x-amz-content-sha256': payloadHash,
    'x-amz-date': amzDate
  };

  const canonicalHeaders = Object.keys(headers)
    .sort()
    .map(k => `${k}:${headers[k]}\n`)
    .join('');

  const signedHeaders = Object.keys(headers).sort().join(';');

  const canonicalRequest = [
    'PUT',
    canonicalUri,
    '', // canonical query string
    canonicalHeaders,
    signedHeaders,
    payloadHash
  ].join('\n');

  const credentialScope = `${dateStamp}/${region}/${service}/aws4_request`;
  const stringToSign = [
    'AWS4-HMAC-SHA256',
    amzDate,
    credentialScope,
    hash(canonicalRequest)
  ].join('\n');

  const kDate = hmac('AWS4' + SECRET_ACCESS_KEY, dateStamp);
  const kRegion = hmac(kDate, region);
  const kService = hmac(kRegion, service);
  const kSigning = hmac(kService, 'aws4_request');
  const signature = crypto.createHmac('sha256', kSigning).update(stringToSign).digest('hex');

  const authHeader = `AWS4-HMAC-SHA256 Credential=${ACCESS_KEY_ID}/${credentialScope}, SignedHeaders=${signedHeaders}, Signature=${signature}`;

  const requestHeaders = {
    ...headers,
    'Authorization': authHeader
  };

  console.log(`--> Uploading ${objectKey} (${(fileBytes / (1024 * 1024)).toFixed(2)} MB)...`);

  const response = await fetch(endpoint, {
    method: 'PUT',
    headers: requestHeaders,
    body: fileBuffer
  });

  if (!response.ok) {
    const errText = await response.text();
    throw new Error(`Upload failed (${response.status} ${response.statusText}): ${errText}`);
  }

  console.log(`[OK] Successfully uploaded: ${objectKey}`);
}

async function main() {
  const version = process.argv[2] || '3.1.0';
  console.log('========================================================================');
  console.log('       SP Language - Cloudflare R2 Release Uploader (Automated)         ');
  console.log(`                         Version ${version}                             `);
  console.log('========================================================================');
  console.log(`Bucket: ${BUCKET_NAME}`);
  console.log(`Account ID: ${ACCOUNT_ID}`);
  console.log(`Access Key: ${ACCESS_KEY_ID.substring(0, 8)}...`);

  const distDir = path.join(repoRoot, 'dist');
  const versionedZip = path.join(distDir, `sp-${version}-windows-x64.zip`);
  const latestZip = path.join(distDir, 'sp-windows-x64.zip');
  const installPs1 = path.join(repoRoot, 'install.ps1');
  const installSh = path.join(repoRoot, 'install.sh');

  const filesToUpload = [
    { path: versionedZip, key: `sp-${version}-windows-x64.zip`, type: 'application/zip' },
    { path: latestZip, key: 'sp-windows-x64.zip', type: 'application/zip' },
    { path: installPs1, key: 'install.ps1', type: 'text/plain; charset=utf-8' },
    { path: installSh, key: 'install.sh', type: 'text/x-shellscript; charset=utf-8' }
  ];

  const toolchainZip = path.join(distDir, 'toolchain-win64.zip');
  if (fs.existsSync(toolchainZip)) {
    filesToUpload.push({ path: toolchainZip, key: 'toolchain-win64.zip', type: 'application/zip' });
  }

  for (const item of filesToUpload) {
    await uploadFileToR2(item.path, item.key, item.type);
  }

  console.log('\n========================================================================');
  console.log('All SP release assets uploaded to Cloudflare R2 successfully!');
  console.log('Public CDN Endpoints:');
  console.log('  * https://dl.splang.shantopaul.com/install.ps1');
  console.log('  * https://dl.splang.shantopaul.com/install.sh');
  console.log(`  * https://dl.splang.shantopaul.com/sp-${version}-windows-x64.zip`);
  console.log('  * https://dl.splang.shantopaul.com/sp-windows-x64.zip');
  console.log('========================================================================\n');
}

main().catch(err => {
  console.error('[ERROR]', err);
  process.exit(1);
});
