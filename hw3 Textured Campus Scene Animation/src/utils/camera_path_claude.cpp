#ifndef CAMERA_PATH_H
#define CAMERA_PATH_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>

// ========== 關鍵幀結構 ==========
struct CameraKeyframe
{
  glm::vec3 position; // 相機位置
  glm::vec3 lookAt;   // 看向的目標點
  float duration;     // 到達這個關鍵幀需要的時間(秒)

  CameraKeyframe(glm::vec3 pos, glm::vec3 target, float dur = 2.0f)
      : position(pos), lookAt(target), duration(dur) {}
};

// ========== 相機路徑系統 ==========
class CameraPath
{
public:
  std::vector<CameraKeyframe> keyframes;
  int currentKeyframe = 0;
  float currentTime = 0.0f;
  bool isPlaying = false;
  bool loop = false;

  // 添加關鍵幀
  void addKeyframe(glm::vec3 position, glm::vec3 lookAt, float duration = 2.0f)
  {
    keyframes.push_back(CameraKeyframe(position, lookAt, duration));
  }

  // 開始播放
  void play()
  {
    isPlaying = true;
    currentKeyframe = 0;
    currentTime = 0.0f;
  }

  // 暫停
  void pause()
  {
    isPlaying = false;
  }

  // 重置
  void reset()
  {
    currentKeyframe = 0;
    currentTime = 0.0f;
    isPlaying = false;
  }

  // ========== 線性插值 ==========
  glm::vec3 lerp(glm::vec3 a, glm::vec3 b, float t)
  {
    return a + t * (b - a);
  }

  // ========== Catmull-Rom 樣條插值 (平滑曲線) ==========
  glm::vec3 catmullRom(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t)
  {
    float t2 = t * t;
    float t3 = t2 * t;

    return 0.5f * ((2.0f * p1) +
                   (-p0 + p2) * t +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
  }

  // ========== 三次貝茲曲線 ==========
  glm::vec3 bezier(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t)
  {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    return uuu * p0 + 3.0f * uu * t * p1 + 3.0f * u * tt * p2 + ttt * p3;
  }

  // ========== 更新相機 (每幀呼叫) ==========
  void update(float deltaTime, glm::vec3 &outPosition, glm::vec3 &outLookAt,
              bool useBezier = false)
  {
    if (!isPlaying || keyframes.size() < 2)
      return;

    currentTime += deltaTime;

    // 檢查是否到達當前關鍵幀
    if (currentTime >= keyframes[currentKeyframe + 1].duration)
    {
      currentTime = 0.0f;
      currentKeyframe++;

      // 檢查是否結束
      if (currentKeyframe >= (int)keyframes.size() - 1)
      {
        if (loop)
        {
          currentKeyframe = 0;
        }
        else
        {
          isPlaying = false;
          currentKeyframe = keyframes.size() - 2;
          currentTime = keyframes.back().duration;
        }
      }
    }

    // 計算插值比例
    float duration = keyframes[currentKeyframe + 1].duration;
    float t = currentTime / duration;

    // 使用緩動函數 (ease-in-out)
    t = smoothstep(t);

    if (useBezier && keyframes.size() >= 4)
    {
      // 貝茲曲線插值 (需要 4 個控制點)
      int i = currentKeyframe;
      int i0 = std::max(0, i - 1);
      int i1 = i;
      int i2 = i + 1;
      int i3 = std::min((int)keyframes.size() - 1, i + 2);

      outPosition = bezier(
          keyframes[i0].position,
          keyframes[i1].position,
          keyframes[i2].position,
          keyframes[i3].position,
          t);

      outLookAt = bezier(
          keyframes[i0].lookAt,
          keyframes[i1].lookAt,
          keyframes[i2].lookAt,
          keyframes[i3].lookAt,
          t);
    }
    else
    {
      // 線性插值
      outPosition = lerp(
          keyframes[currentKeyframe].position,
          keyframes[currentKeyframe + 1].position,
          t);

      outLookAt = lerp(
          keyframes[currentKeyframe].lookAt,
          keyframes[currentKeyframe + 1].lookAt,
          t);
    }
  }

  // ========== 平滑步進函數 (緩動) ==========
  float smoothstep(float t)
  {
    return t * t * (3.0f - 2.0f * t);
  }

  // ========== 獲取總時長 ==========
  float getTotalDuration()
  {
    float total = 0.0f;
    for (const auto &kf : keyframes)
      total += kf.duration;
    return total;
  }

  // ========== 跳到特定時間 ==========
  void seekToTime(float time)
  {
    currentTime = 0.0f;
    currentKeyframe = 0;

    float accumulated = 0.0f;
    for (size_t i = 1; i < keyframes.size(); i++)
    {
      if (accumulated + keyframes[i].duration > time)
      {
        currentKeyframe = i - 1;
        currentTime = time - accumulated;
        break;
      }
      accumulated += keyframes[i].duration;
    }
  }
};

// ========== 預設場景路徑 ==========
class ScenePaths
{
public:
  // 校園導覽路徑
  static CameraPath createSchoolTour()
  {
    CameraPath path;

    // 入口鳥瞰
    path.addKeyframe(
        glm::vec3(0.0f, 5.0f, 10.0f), // 高空俯視
        glm::vec3(0.0f, 0.0f, 0.0f),  // 看向中心
        3.0f);

    // 降落到地面
    path.addKeyframe(
        glm::vec3(0.0f, 1.5f, 5.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        2.0f);

    // 向前移動
    path.addKeyframe(
        glm::vec3(0.0f, 1.5f, -5.0f),
        glm::vec3(0.0f, 1.0f, -10.0f),
        4.0f);

    // 右轉環繞
    path.addKeyframe(
        glm::vec3(5.0f, 1.5f, -5.0f),
        glm::vec3(0.0f, 1.0f, -5.0f),
        3.0f);

    // 繼續環繞
    path.addKeyframe(
        glm::vec3(5.0f, 1.5f, 5.0f),
        glm::vec3(0.0f, 1.0f, 5.0f),
        4.0f);

    // 返回起點
    path.addKeyframe(
        glm::vec3(0.0f, 1.5f, 10.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        3.0f);

    return path;
  }

  // 圓形環繞
  static CameraPath createCircularPath(glm::vec3 center, float radius, float height, int segments = 16)
  {
    CameraPath path;

    for (int i = 0; i <= segments; i++)
    {
      float angle = (float)i / segments * 2.0f * 3.14159f;
      glm::vec3 pos = center + glm::vec3(
                                   cos(angle) * radius,
                                   height,
                                   sin(angle) * radius);

      path.addKeyframe(pos, center, 2.0f);
    }

    return path;
  }

  // 螺旋上升
  static CameraPath createSpiralPath(glm::vec3 center, float radius, float startHeight,
                                     float endHeight, int segments = 16)
  {
    CameraPath path;

    for (int i = 0; i <= segments; i++)
    {
      float t = (float)i / segments;
      float angle = t * 4.0f * 3.14159f; // 2圈
      float height = startHeight + (endHeight - startHeight) * t;

      glm::vec3 pos = center + glm::vec3(
                                   cos(angle) * radius,
                                   height,
                                   sin(angle) * radius);

      path.addKeyframe(pos, center, 1.5f);
    }

    return path;
  }
};

#endif // CAMERA_PATH_H