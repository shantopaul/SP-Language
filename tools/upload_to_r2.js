/**
 * ==============================================================================
 * SP Language - Cloudflare R2 S3-Compatible Direct Release Uploader
 * ==============================================================================
 * Uses pure Node.js native standard libraries (crypto, fs, path, fetch).
 * Zero external npm dependencies.
 *
 * Reads credentials securely from:
 *   1. .env file in repository root
 *   2. Environment variables (process.env)
 *
 * Usage:
 *   node tools/upload_to_r2.js [--file path/to/file]
 * ==============================================================================
 */

import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..');

// 1. Simple .env file parser
function loadEnv() {
  const envPath = path.join(repoRoot, '.env');
  if (fs.existsSync(envPath)) {
    const lines = fs.readFileSync(envPath, 'utf8').split('\n');
    for (const rawLine of lines) {
      const line = rawLine.trim();
      if (!line || line.startsWith('#')) continue;
      const eqIdx = line.indexOf('=');
      if (eqIdx !== -1) {
        const key = line.slice(0, eqIdx).trim();
        let val = line.slice(eqIdx + 1).trim();
        if ((val.startsWith('"') && val.endsWith('"')) || (val.startsWith("'") && val.endsWith("'"))) {
          val = val.slice(1, -1);
        }
        if (!process.env[key]) {
          process.env[key] = val;
        }
      }
    }
  }
}

loadEnv();

const ACCOUNT_ID = process.env.R2_ACCOUNT_ID || '3b56f1445438ff6041fee436fe29b097';
const BUCKET_NAME = process.env.R2_BUCKET_NAME || 'splang-releases';
const ACCESS_KEY_ID = process.env.R2_ACCESS_KEY_ID;
const SECRET_ACCESS_KEY = process.env.R2_SECRET_ACCESS_KEY;
const REGION = 'auto';
const SERVICE = 's3';

console.log('========================================================================');
console.log('       SP Programming Language - Cloudflare R2 Direct Uploader         ');
console.log('========================================================================');
console.log(`  Account ID  : ${ACCOUNT_ID}`);
console.log(`  Bucket Name : ${BUCKET_NAME}`);
console.log(`  S3 Endpoint : https://${ACCOUNT_ID}.r2.cloudflarestorage.com`);

if (!ACCESS_KEY_ID || !SECRET_ACCESS_KEY || ACCESS_KEY_ID === 'your_access_key_id_here') {
  console.log('\n[!] Missing or incomplete R2 API credentials.');
  console.log('    Please set them in a .env file in the repository root:\n');
  console.log('    R2_ACCOUNT_ID=' + ACCOUNT_ID);
  console.log('    R2_BUCKET_NAME=' + BUCKET_NAME);
  console.log('    R2_ACCESS_KEY_ID=<Your R2 Access Key ID>');
  console.log('    R2_SECRET_ACCESS_KEY=<Your R2 Secret Access Key>\n');
  console.log('    How to get them:');
  console.log('    1. Go to Cloudflare Dashboard -> R2 -> "Manage R2 API Tokens"');
  console.log('    2. Click "Create Account API token"');
  console.log('    3. Permission: Object Read & Write');
  console.log('    4. Copy the "Access Key ID" and "Secret Access Key" into your .env file.\n');
  process.exit(1);
}

// 2. AWS SigV4 implementation
function hmacSha256(key, data) {
  return crypto.createHmac('sha256', key).update(data).digest();
}

function sha256Hex(data) {
  return crypto.createHash('sha256').update(data).digest('hex');
}

function getSigningKey(secretKey, dateStamp, region, service) {
  const kDate = hmacSha256('AWS4' + secretKey, dateStamp);
  const kRegion = hmacSha256(kDate, region);
  const kService = hmacSha256(kRegion, service);
  return hmacSha256(kService, 'aws4_request');
}

function getMimeType(filePath) {
  const ext = path.extname(filePath).toLowerCase();
  switch (ext) {
    case '.zip': return 'application/zip';
    case '.exe': return 'application/octet-stream';
    case '.vsix': return 'application/octet-stream';
    case '.ps1': return 'text/plain; charset=utf-8';
    case '.sh':  return 'text/plain; charset=utf-8';
    case '.json': return 'application/json';
    case '.tar.gz': case '.tgz': return 'application/gzip';
    default: return 'application/octet-stream';
  }
}

async function uploadFileToR2(filePath, objectKey) {
  const fileBuffer = fs.readFileSync(filePath);
  const fileSizeMB = (fileBuffer.length / (1024 * 1024)).toFixed(2);
  const mimeType = getMimeType(filePath);
  const payloadHash = sha256Hex(fileBuffer);

  const now = new Date();
  const amzDate = now.toISOString().replace(/[:-]|\.\d{3}/g, '');
  const dateStamp = amzDate.substring(0, 8);

  const host = `${ACCOUNT_ID}.r2.cloudflarestorage.com`;
  // Canonical URI path: /<bucket>/<key>
  const canonicalUri = `/${BUCKET_NAME}/${objectKey}`;

  // Canonical headers (must be sorted alphabetically, lowercase)
  const canonicalHeaders = 
    `content-length:${fileBuffer.length}\n` +
    `content-type:${mimeType}\n` +
    `host:${host}\n` +
    `x-amz-content-sha256:${payloadHash}\n` +
    `x-amz-date:${amzDate}\n`;

  const signedHeaders = 'content-length;content-type;host;x-amz-content-sha256;x-amz-date';

  const canonicalRequest = 
    `PUT\n` +
    `${canonicalUri}\n\n` +
    `${canonicalHeaders}\n` +
    `${signedHeaders}\n` +
    `${payloadHash}`;

  const credentialScope = `${dateStamp}/${REGION}/${SERVICE}/aws4_request`;
  const stringToSign = 
    `AWS4-HMAC-SHA256\n` +
    `${amzDate}\n` +
    `${credentialScope}\n` +
    `${sha256Hex(canonicalRequest)}`;

  const signingKey = getSigningKey(SECRET_ACCESS_KEY, dateStamp, REGION, SERVICE);
  const signature = crypto.createHmac('sha256', signingKey).update(stringToSign).digest('hex');

  const authHeader = `AWS4-HMAC-SHA256 Credential=${ACCESS_KEY_ID}/${credentialScope}, SignedHeaders=${signedHeaders}, Signature=${signature}`;

  const targetUrl = `https://${host}${canonicalUri}`;
  console.log(`\n[*] Uploading '${objectKey}' (${fileSizeMB} MB) to R2...`);

  try {
    const res = await fetch(targetUrl, {
      method: 'PUT',
      headers: {
        'Host': host,
        'Content-Type': mimeType,
        'Content-Length': fileBuffer.length.toString(),
        'x-amz-date': amzDate,
        'x-amz-content-sha256': payloadHash,
        'Authorization': authHeader
      },
      body: fileBuffer
    });

    if (res.ok) {
      console.log(`    [✓ SUCCESS] Uploaded ${objectKey} (HTTP ${res.status})`);
      console.log(`    Public URL: https://dl.splang.shantopaul.com/${objectKey}`);
      return true;
    } else {
      const errText = await res.text();
      console.error(`    [!] Upload failed with HTTP ${res.status}: ${errText}`);
      return false;
    }
  } catch (err) {
    console.error(`    [!] Network error uploading ${objectKey}:`, err.message);
    return false;
  }
}

async function main() {
  const filesToUpload = [];

  // Default files to distribute
  const defaultZip = path.join(repoRoot, 'dist', 'sp-1.0.0-windows-x64.zip');
  if (fs.existsSync(defaultZip)) {
    filesToUpload.push({ path: defaultZip, key: 'sp-1.0.0-windows-x64.zip' });
  }

  const installPs1 = path.join(repoRoot, 'install.ps1');
  if (fs.existsSync(installPs1)) {
    filesToUpload.push({ path: installPs1, key: 'install.ps1' });
  }

  const installSh = path.join(repoRoot, 'splang', 'public', 'install.sh');
  if (fs.existsSync(installSh)) {
    filesToUpload.push({ path: installSh, key: 'install.sh' });
  }

  const setupExe = path.join(repoRoot, 'dist', 'sp-setup.exe');
  if (fs.existsSync(setupExe)) {
    filesToUpload.push({ path: setupExe, key: 'sp-setup.exe' });
  }

  const vsixExt = path.join(repoRoot, 'dist', 'sp-language-support-1.0.0.vsix');
  if (fs.existsSync(vsixExt)) {
    filesToUpload.push({ path: vsixExt, key: 'sp-language-support-1.0.0.vsix' });
    filesToUpload.push({ path: vsixExt, key: 'sp-vscode.vsix' });
  }

  if (filesToUpload.length === 0) {
    console.log('[!] No release artifacts found to upload. Please build dist/ artifacts first.');
    process.exit(1);
  }

  console.log(`Found ${filesToUpload.length} file(s) queued for upload:`);
  for (const f of filesToUpload) {
    console.log(`  - ${f.key} (from ${path.relative(repoRoot, f.path)})`);
  }

  let allSuccess = true;
  for (const f of filesToUpload) {
    const success = await uploadFileToR2(f.path, f.key);
    if (!success) allSuccess = false;
  }

  if (allSuccess) {
    console.log('\n========================================================================');
    console.log('   All release artifacts uploaded to Cloudflare R2 successfully!       ');
    console.log('========================================================================');
    console.log('Available at:');
    console.log('  https://dl.splang.shantopaul.com/sp-1.0.0-windows-x64.zip');
    console.log('  https://dl.splang.shantopaul.com/install.ps1');
    console.log('========================================================================\n');
  } else {
    console.log('\n[!] Some uploads encountered errors. Check output above.');
    process.exit(1);
  }
}

main();
