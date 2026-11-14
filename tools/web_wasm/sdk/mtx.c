/* MTX/VEC/QUAT: plain C versions of the paired-single library. */
#include <math.h>
#include <string.h>
#include "port_sdk.h"

void PSMTXIdentity(Mtx m)
{
    memset(m, 0, sizeof(Mtx));
    m[0][0] = m[1][1] = m[2][2] = 1.0f;
}

void PSMTXCopy(const Mtx src, Mtx dst)
{
    if (src != dst)
        memcpy(dst, src, sizeof(Mtx));
}

void PSMTXConcat(const Mtx a, const Mtx b, Mtx ab)
{
    Mtx t;
    int i;
    for (i = 0; i < 3; i++) {
        t[i][0] = a[i][0] * b[0][0] + a[i][1] * b[1][0] + a[i][2] * b[2][0];
        t[i][1] = a[i][0] * b[0][1] + a[i][1] * b[1][1] + a[i][2] * b[2][1];
        t[i][2] = a[i][0] * b[0][2] + a[i][1] * b[1][2] + a[i][2] * b[2][2];
        t[i][3] = a[i][0] * b[0][3] + a[i][1] * b[1][3] + a[i][2] * b[2][3] + a[i][3];
    }
    memcpy(ab, t, sizeof(Mtx));
}

void PSMTXTranspose(const Mtx src, Mtx xPose)
{
    Mtx t;
    int i, j;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            t[i][j] = src[j][i];
    t[0][3] = t[1][3] = t[2][3] = 0.0f;
    memcpy(xPose, t, sizeof(Mtx));
}

u32 PSMTXInverse(const Mtx src, Mtx inv)
{
    Mtx t;
    f32 det = src[0][0] * src[1][1] * src[2][2] + src[0][1] * src[1][2] * src[2][0] +
              src[0][2] * src[1][0] * src[2][1] - src[2][0] * src[1][1] * src[0][2] -
              src[1][0] * src[0][1] * src[2][2] - src[0][0] * src[2][1] * src[1][2];
    if (det == 0.0f)
        return 0;
    det = 1.0f / det;
    t[0][0] = (src[1][1] * src[2][2] - src[2][1] * src[1][2]) * det;
    t[0][1] = -(src[0][1] * src[2][2] - src[2][1] * src[0][2]) * det;
    t[0][2] = (src[0][1] * src[1][2] - src[1][1] * src[0][2]) * det;
    t[1][0] = -(src[1][0] * src[2][2] - src[2][0] * src[1][2]) * det;
    t[1][1] = (src[0][0] * src[2][2] - src[2][0] * src[0][2]) * det;
    t[1][2] = -(src[0][0] * src[1][2] - src[1][0] * src[0][2]) * det;
    t[2][0] = (src[1][0] * src[2][1] - src[2][0] * src[1][1]) * det;
    t[2][1] = -(src[0][0] * src[2][1] - src[2][0] * src[0][1]) * det;
    t[2][2] = (src[0][0] * src[1][1] - src[1][0] * src[0][1]) * det;
    t[0][3] = -t[0][0] * src[0][3] - t[0][1] * src[1][3] - t[0][2] * src[2][3];
    t[1][3] = -t[1][0] * src[0][3] - t[1][1] * src[1][3] - t[1][2] * src[2][3];
    t[2][3] = -t[2][0] * src[0][3] - t[2][1] * src[1][3] - t[2][2] * src[2][3];
    memcpy(inv, t, sizeof(Mtx));
    return 1;
}

void PSMTXMultVec(const Mtx m, const Vec *src, Vec *dst)
{
    Vec v = *src;
    dst->x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3];
    dst->y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3];
    dst->z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3];
}

void PSMTXMultVecSR(const Mtx m, const Vec *src, Vec *dst)
{
    Vec v = *src;
    dst->x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z;
    dst->y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z;
    dst->z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z;
}

void PSMTXMultVecArraySR(const Mtx m, const Vec *srcBase, Vec *dstBase, u32 count)
{
    u32 i;
    for (i = 0; i < count; i++)
        PSMTXMultVecSR(m, &srcBase[i], &dstBase[i]);
}

void PSMTX44MultVec(const Mtx44 m, const Vec *src, Vec *dst)
{
    Vec v = *src;
    f32 w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3];
    w = 1.0f / w;
    dst->x = (m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3]) * w;
    dst->y = (m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3]) * w;
    dst->z = (m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3]) * w;
}

void PSMTXQuat(Mtx m, const Quaternion *q)
{
    f32 s = 2.0f / (q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w);
    f32 xs = q->x * s, ys = q->y * s, zs = q->z * s;
    f32 wx = q->w * xs, wy = q->w * ys, wz = q->w * zs;
    f32 xx = q->x * xs, xy = q->x * ys, xz = q->x * zs;
    f32 yy = q->y * ys, yz = q->y * zs, zz = q->z * zs;
    m[0][0] = 1.0f - (yy + zz);
    m[0][1] = xy - wz;
    m[0][2] = xz + wy;
    m[0][3] = 0.0f;
    m[1][0] = xy + wz;
    m[1][1] = 1.0f - (xx + zz);
    m[1][2] = yz - wx;
    m[1][3] = 0.0f;
    m[2][0] = xz - wy;
    m[2][1] = yz + wx;
    m[2][2] = 1.0f - (xx + yy);
    m[2][3] = 0.0f;
}

void PSMTXRotTrig(Mtx m, char axis, f32 sinA, f32 cosA)
{
    PSMTXIdentity(m);
    switch (axis) {
    case 'x':
    case 'X':
        m[1][1] = cosA; m[1][2] = -sinA;
        m[2][1] = sinA; m[2][2] = cosA;
        break;
    case 'y':
    case 'Y':
        m[0][0] = cosA; m[0][2] = sinA;
        m[2][0] = -sinA; m[2][2] = cosA;
        break;
    case 'z':
    case 'Z':
        m[0][0] = cosA; m[0][1] = -sinA;
        m[1][0] = sinA; m[1][1] = cosA;
        break;
    }
}

void PSMTXRotRad(Mtx m, char axis, f32 rad) { PSMTXRotTrig(m, axis, sinf(rad), cosf(rad)); }

void PSMTXRotAxisRad(Mtx m, const Vec *axis, f32 rad)
{
    Vec n;
    f32 s = sinf(rad), c = cosf(rad), t = 1.0f - c;
    PSVECNormalize(axis, &n);
    m[0][0] = t * n.x * n.x + c;
    m[0][1] = t * n.x * n.y - s * n.z;
    m[0][2] = t * n.x * n.z + s * n.y;
    m[0][3] = 0.0f;
    m[1][0] = t * n.x * n.y + s * n.z;
    m[1][1] = t * n.y * n.y + c;
    m[1][2] = t * n.y * n.z - s * n.x;
    m[1][3] = 0.0f;
    m[2][0] = t * n.x * n.z - s * n.y;
    m[2][1] = t * n.y * n.z + s * n.x;
    m[2][2] = t * n.z * n.z + c;
    m[2][3] = 0.0f;
}

void PSMTXTrans(Mtx m, f32 xT, f32 yT, f32 zT)
{
    PSMTXIdentity(m);
    m[0][3] = xT;
    m[1][3] = yT;
    m[2][3] = zT;
}

void PSMTXTransApply(const Mtx src, Mtx dst, f32 xT, f32 yT, f32 zT)
{
    PSMTXCopy(src, dst);
    dst[0][3] += xT;
    dst[1][3] += yT;
    dst[2][3] += zT;
}

void PSMTXScale(Mtx m, f32 xS, f32 yS, f32 zS)
{
    memset(m, 0, sizeof(Mtx));
    m[0][0] = xS;
    m[1][1] = yS;
    m[2][2] = zS;
}

void PSMTXScaleApply(const Mtx src, Mtx dst, f32 xS, f32 yS, f32 zS)
{
    int j;
    for (j = 0; j < 4; j++) {
        dst[0][j] = src[0][j] * xS;
        dst[1][j] = src[1][j] * yS;
        dst[2][j] = src[2][j] * zS;
    }
}

void PSVECAdd(const Vec *a, const Vec *b, Vec *ab)
{
    ab->x = a->x + b->x;
    ab->y = a->y + b->y;
    ab->z = a->z + b->z;
}

void PSVECSubtract(const Vec *a, const Vec *b, Vec *a_b)
{
    a_b->x = a->x - b->x;
    a_b->y = a->y - b->y;
    a_b->z = a->z - b->z;
}

void PSVECScale(const Vec *src, Vec *dst, f32 scale)
{
    dst->x = src->x * scale;
    dst->y = src->y * scale;
    dst->z = src->z * scale;
}

f32 PSVECDotProduct(const Vec *a, const Vec *b) { return a->x * b->x + a->y * b->y + a->z * b->z; }
f32 PSVECSquareMag(const Vec *v) { return v->x * v->x + v->y * v->y + v->z * v->z; }
f32 PSVECMag(const Vec *v) { return sqrtf(PSVECSquareMag(v)); }

void PSVECNormalize(const Vec *src, Vec *dst)
{
    f32 inv = 1.0f / sqrtf(PSVECSquareMag(src));
    PSVECScale(src, dst, inv);
}

void PSVECCrossProduct(const Vec *a, const Vec *b, Vec *axb)
{
    Vec t;
    t.x = a->y * b->z - a->z * b->y;
    t.y = a->z * b->x - a->x * b->z;
    t.z = a->x * b->y - a->y * b->x;
    *axb = t;
}

f32 PSVECDistance(const Vec *a, const Vec *b)
{
    Vec d;
    PSVECSubtract(a, b, &d);
    return PSVECMag(&d);
}

void PSQUATMultiply(const Quaternion *p, const Quaternion *q, Quaternion *pq)
{
    Quaternion t;
    t.x = p->w * q->x + p->x * q->w + p->y * q->z - p->z * q->y;
    t.y = p->w * q->y + p->y * q->w + p->z * q->x - p->x * q->z;
    t.z = p->w * q->z + p->z * q->w + p->x * q->y - p->y * q->x;
    t.w = p->w * q->w - p->x * q->x - p->y * q->y - p->z * q->z;
    *pq = t;
}

void PSQUATScale(const Quaternion *q, Quaternion *r, f32 scale)
{
    r->x = q->x * scale;
    r->y = q->y * scale;
    r->z = q->z * scale;
    r->w = q->w * scale;
}

f32 PSQUATDotProduct(const Quaternion *p, const Quaternion *q)
{
    return p->x * q->x + p->y * q->y + p->z * q->z + p->w * q->w;
}

/* The decomp header orders the parameters to match PPC register assignment;
 * the meaning is QUATRotAxisRad(Quaternion *out, const Vec *axis, f32 angle). */
void C_QUATRotAxisRad(float r, float *axis, Vec *rad)
{
    Quaternion *q = (Quaternion *)axis;
    Vec n;
    f32 half = r * 0.5f, s = sinf(half);
    PSVECNormalize(rad, &n);
    q->x = s * n.x;
    q->y = s * n.y;
    q->z = s * n.z;
    q->w = cosf(half);
}

void C_MTXLookAt(Mtx m, const Vec *camPos, const Vec *camUp, const Vec *target)
{
    Vec look, right, up;
    look.x = camPos->x - target->x;
    look.y = camPos->y - target->y;
    look.z = camPos->z - target->z;
    PSVECNormalize(&look, &look);
    PSVECCrossProduct(camUp, &look, &right);
    PSVECNormalize(&right, &right);
    PSVECCrossProduct(&look, &right, &up);
    m[0][0] = right.x; m[0][1] = right.y; m[0][2] = right.z;
    m[0][3] = -(camPos->x * right.x + camPos->y * right.y + camPos->z * right.z);
    m[1][0] = up.x; m[1][1] = up.y; m[1][2] = up.z;
    m[1][3] = -(camPos->x * up.x + camPos->y * up.y + camPos->z * up.z);
    m[2][0] = look.x; m[2][1] = look.y; m[2][2] = look.z;
    m[2][3] = -(camPos->x * look.x + camPos->y * look.y + camPos->z * look.z);
}

void C_MTXFrustum(Mtx44 m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f)
{
    f32 tmp = 1.0f / (r - l);
    memset(m, 0, sizeof(Mtx44));
    m[0][0] = 2 * n * tmp;
    m[0][2] = (r + l) * tmp;
    tmp = 1.0f / (t - b);
    m[1][1] = 2 * n * tmp;
    m[1][2] = (t + b) * tmp;
    tmp = 1.0f / (f - n);
    m[2][2] = -n * tmp;
    m[2][3] = -(f * n) * tmp;
    m[3][2] = -1.0f;
}

void C_MTXPerspective(Mtx44 m, f32 fovY, f32 aspect, f32 n, f32 f)
{
    f32 cot = 1.0f / tanf(fovY * 0.5f * (3.14159265358979323846f / 180.0f));
    f32 tmp = 1.0f / (f - n);
    memset(m, 0, sizeof(Mtx44));
    m[0][0] = cot / aspect;
    m[1][1] = cot;
    m[2][2] = -n * tmp;
    m[2][3] = -(f * n) * tmp;
    m[3][2] = -1.0f;
}

void C_MTXOrtho(Mtx44 m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f)
{
    f32 tmp = 1.0f / (r - l);
    memset(m, 0, sizeof(Mtx44));
    m[0][0] = 2.0f * tmp;
    m[0][3] = -(r + l) * tmp;
    tmp = 1.0f / (t - b);
    m[1][1] = 2.0f * tmp;
    m[1][3] = -(t + b) * tmp;
    tmp = 1.0f / (f - n);
    m[2][2] = -1.0f * tmp;
    m[2][3] = -f * tmp;
    m[3][3] = 1.0f;
}

void C_MTXLightPerspective(Mtx m, f32 fovY, f32 aspect, f32 scaleS, f32 scaleT, f32 transS, f32 transT)
{
    f32 cot = 1.0f / tanf(fovY * 0.5f * (3.14159265358979323846f / 180.0f));
    memset(m, 0, sizeof(Mtx));
    m[0][0] = (cot / aspect) * scaleS;
    m[0][2] = -transS;
    m[1][1] = cot * scaleT;
    m[1][2] = -transT;
    m[2][2] = -1.0f;
}

void C_MTXLightOrtho(Mtx m, f32 t, f32 b, f32 l, f32 r, f32 scaleS, f32 scaleT, f32 transS, f32 transT)
{
    f32 tmp = 1.0f / (r - l);
    memset(m, 0, sizeof(Mtx));
    m[0][0] = 2.0f * tmp * scaleS;
    m[0][3] = -(r + l) * tmp * scaleS + transS;
    tmp = 1.0f / (t - b);
    m[1][1] = 2.0f * tmp * scaleT;
    m[1][3] = -(t + b) * tmp * scaleT + transT;
    m[2][3] = 1.0f;
}
