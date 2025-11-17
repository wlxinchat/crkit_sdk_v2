/**
 * @file crkit_api.h
 * @brief CRKit AI Inference SDK - Standard C API
 * @version 1.0.0
 *
 * 通用AI推理SDK标准接口
 * 支持多种推理后端(TensorRT, ONNX Runtime等)
 * 适用于工业质检等实时AI推理场景
 */

#ifndef CRKIT_API_H
#define CRKIT_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==================== 版本信息 ==================== */
#define CRKIT_VERSION_MAJOR 1
#define CRKIT_VERSION_MINOR 0
#define CRKIT_VERSION_PATCH 0

/* ==================== 导出符号 ==================== */
#ifdef _WIN32
    #ifdef CRKIT_BUILD_SHARED
        #define CRKIT_API __declspec(dllexport)
    #else
        #define CRKIT_API __declspec(dllimport)
    #endif
#else
    #define CRKIT_API __attribute__((visibility("default")))
#endif

/* ==================== 错误码定义 ==================== */
typedef enum {
    CRKIT_SUCCESS = 0,                    /**< 成功 */
    CRKIT_ERROR_INVALID_PARAM = -1,       /**< 无效参数 */
    CRKIT_ERROR_OUT_OF_MEMORY = -2,       /**< 内存不足 */
    CRKIT_ERROR_MODEL_LOAD_FAILED = -3,   /**< 模型加载失败 */
    CRKIT_ERROR_INFERENCE_FAILED = -4,    /**< 推理执行失败 */
    CRKIT_ERROR_NOT_INITIALIZED = -5,     /**< 未初始化 */
    CRKIT_ERROR_TIMEOUT = -6,             /**< 超时 */
    CRKIT_ERROR_DEVICE_ERROR = -7,        /**< 设备错误 */
    CRKIT_ERROR_UNSUPPORTED = -8,         /**< 不支持的操作 */
    CRKIT_ERROR_IO_ERROR = -9,            /**< IO错误 */
    CRKIT_ERROR_INVALID_MODEL = -10       /**< 无效的模型 */
} CRKitStatus;

/* ==================== 推理后端类型 ==================== */
typedef enum {
    CRKIT_BACKEND_TENSORRT = 0,      /**< TensorRT后端 */
    CRKIT_BACKEND_ONNXRUNTIME = 1,   /**< ONNX Runtime后端 */
    CRKIT_BACKEND_OPENVINO = 2,      /**< OpenVINO后端 */
    CRKIT_BACKEND_AUTO = 99          /**< 自动选择 */
} CRKitBackendType;

/* ==================== 设备类型 ==================== */
typedef enum {
    CRKIT_DEVICE_CPU = 0,            /**< CPU设备 */
    CRKIT_DEVICE_GPU = 1             /**< GPU设备 */
} CRKitDeviceType;

/* ==================== 数据类型 ==================== */
typedef enum {
    CRKIT_DTYPE_FLOAT32 = 0,         /**< 32位浮点 */
    CRKIT_DTYPE_FLOAT16 = 1,         /**< 16位浮点 */
    CRKIT_DTYPE_INT8 = 2,            /**< 8位整数 */
    CRKIT_DTYPE_UINT8 = 3            /**< 8位无符号整数 */
} CRKitDataType;

/* ==================== 图像格式 ==================== */
typedef enum {
    CRKIT_IMAGE_FORMAT_RGB = 0,      /**< RGB格式 */
    CRKIT_IMAGE_FORMAT_BGR = 1,      /**< BGR格式 */
    CRKIT_IMAGE_FORMAT_RGBA = 2,     /**< RGBA格式 */
    CRKIT_IMAGE_FORMAT_BGRA = 3,     /**< BGRA格式 */
    CRKIT_IMAGE_FORMAT_GRAY = 4      /**< 灰度图 */
} CRKitImageFormat;

/* ==================== 不透明句柄 ==================== */
typedef void* CRKitEngine;           /**< 推理引擎句柄 */
typedef void* CRKitModel;            /**< 模型句柄 */
typedef void* CRKitInputStream;      /**< 输入流句柄 */
typedef void* CRKitAsyncTask;        /**< 异步任务句柄 */

/* ==================== 引擎配置 ==================== */
typedef struct {
    CRKitBackendType backend;        /**< 推理后端类型 */
    CRKitDeviceType device;          /**< 设备类型 */
    int device_id;                   /**< 设备ID (GPU编号) */
    int max_batch_size;              /**< 最大批处理大小 */
    bool enable_fp16;                /**< 是否启用FP16 */
    int num_worker_threads;          /**< 工作线程数 */
    int max_workspace_mb;            /**< 最大工作空间(MB) */
    const char* log_level;           /**< 日志级别: DEBUG/INFO/WARN/ERROR */
} CRKitEngineConfig;

/* ==================== 模型配置 ==================== */
typedef struct {
    const char* model_path;          /**< 模型文件路径 */
    int input_width;                 /**< 输入图像宽度 */
    int input_height;                /**< 输入图像高度 */
    int num_classes;                 /**< 类别数量 */
    float conf_threshold;            /**< 置信度阈值 */
    float nms_threshold;             /**< NMS阈值 */
    const char** class_names;        /**< 类别名称数组 */
    bool keep_aspect_ratio;          /**< 是否保持宽高比 */
    float mean[3];                   /**< 均值归一化 */
    float std[3];                    /**< 标准差归一化 */
} CRKitModelConfig;

/* ==================== 图像数据 ==================== */
typedef struct {
    uint8_t* data;                   /**< 图像数据指针 */
    int width;                       /**< 图像宽度 */
    int height;                      /**< 图像高度 */
    int channels;                    /**< 通道数 */
    CRKitImageFormat format;         /**< 图像格式 */
    int stride;                      /**< 行跨度(字节) */
} CRKitImage;

/* ==================== 检测框结果 ==================== */
typedef struct {
    float x;                         /**< 左上角x坐标 */
    float y;                         /**< 左上角y坐标 */
    float width;                     /**< 宽度 */
    float height;                    /**< 高度 */
    int class_id;                    /**< 类别ID */
    float confidence;                /**< 置信度 */
    const char* class_name;          /**< 类别名称 */
} CRKitDetection;

/* ==================== 推理结果 ==================== */
typedef struct {
    CRKitDetection* detections;      /**< 检测结果数组 */
    int num_detections;              /**< 检测数量 */
    float preprocess_time_ms;        /**< 预处理耗时(毫秒) */
    float inference_time_ms;         /**< 推理耗时(毫秒) */
    float postprocess_time_ms;       /**< 后处理耗时(毫秒) */
    int64_t timestamp;               /**< 时间戳 */
} CRKitResult;

/* ==================== 异步回调函数 ==================== */
/**
 * @brief 异步推理完成回调函数
 * @param result 推理结果指针
 * @param user_data 用户自定义数据
 */
typedef void (*CRKitAsyncCallback)(const CRKitResult* result, void* user_data);

/* ==================== 核心API ==================== */

/**
 * @brief 获取SDK版本号
 * @return 版本字符串 (例如: "1.0.0")
 */
CRKIT_API const char* crkit_get_version(void);

/**
 * @brief 获取最后一次错误信息
 * @return 错误描述字符串
 */
CRKIT_API const char* crkit_get_last_error(void);

/**
 * @brief 创建默认引擎配置
 * @param config 配置结构体指针
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_create_default_engine_config(CRKitEngineConfig* config);

/**
 * @brief 创建默认模型配置
 * @param config 配置结构体指针
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_create_default_model_config(CRKitModelConfig* config);

/**
 * @brief 创建推理引擎
 * @param engine 引擎句柄指针
 * @param config 引擎配置
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_create_engine(CRKitEngine* engine,
                                          const CRKitEngineConfig* config);

/**
 * @brief 销毁推理引擎
 * @param engine 引擎句柄
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_destroy_engine(CRKitEngine engine);

/**
 * @brief 加载模型
 * @param engine 引擎句柄
 * @param model 模型句柄指针
 * @param config 模型配置
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_load_model(CRKitEngine engine,
                                       CRKitModel* model,
                                       const CRKitModelConfig* config);

/**
 * @brief 卸载模型
 * @param model 模型句柄
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_unload_model(CRKitModel model);

/**
 * @brief 模型热更新
 * @param model 当前模型句柄
 * @param config 新模型配置
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_hot_update_model(CRKitModel model,
                                             const CRKitModelConfig* config);

/* ==================== 同步推理API ==================== */

/**
 * @brief 单图像同步推理
 * @param model 模型句柄
 * @param image 输入图像
 * @param result 推理结果指针
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_infer(CRKitModel model,
                                  const CRKitImage* image,
                                  CRKitResult** result);

/**
 * @brief 批量图像同步推理
 * @param model 模型句柄
 * @param images 输入图像数组
 * @param num_images 图像数量
 * @param results 推理结果数组指针
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_infer_batch(CRKitModel model,
                                        const CRKitImage* images,
                                        int num_images,
                                        CRKitResult*** results);

/**
 * @brief 从文件路径推理
 * @param model 模型句柄
 * @param image_path 图像文件路径
 * @param result 推理结果指针
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_infer_from_file(CRKitModel model,
                                            const char* image_path,
                                            CRKitResult** result);

/* ==================== 异步推理API ==================== */

/**
 * @brief 提交异步推理任务
 * @param model 模型句柄
 * @param image 输入图像
 * @param callback 回调函数
 * @param user_data 用户数据
 * @param task 异步任务句柄指针
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_infer_async(CRKitModel model,
                                        const CRKitImage* image,
                                        CRKitAsyncCallback callback,
                                        void* user_data,
                                        CRKitAsyncTask* task);

/**
 * @brief 等待异步任务完成
 * @param task 异步任务句柄
 * @param timeout_ms 超时时间(毫秒), -1表示永久等待
 * @param result 推理结果指针
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_wait_async_task(CRKitAsyncTask task,
                                            int timeout_ms,
                                            CRKitResult** result);

/**
 * @brief 取消异步任务
 * @param task 异步任务句柄
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_cancel_async_task(CRKitAsyncTask task);

/* ==================== 视频流API ==================== */

/**
 * @brief 打开视频流
 * @param stream 输入流句柄指针
 * @param source 视频源 (文件路径/RTSP URL/设备ID)
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_open_stream(CRKitInputStream* stream,
                                        const char* source);

/**
 * @brief 从视频流读取一帧
 * @param stream 输入流句柄
 * @param image 图像数据指针
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_read_frame(CRKitInputStream stream,
                                       CRKitImage** image);

/**
 * @brief 关闭视频流
 * @param stream 输入流句柄
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_close_stream(CRKitInputStream stream);

/**
 * @brief 视频流推理 (持续读取并推理)
 * @param model 模型句柄
 * @param stream 输入流句柄
 * @param callback 每帧推理完成回调
 * @param user_data 用户数据
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_infer_stream(CRKitModel model,
                                         CRKitInputStream stream,
                                         CRKitAsyncCallback callback,
                                         void* user_data);

/**
 * @brief 停止视频流推理
 * @param model 模型句柄
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_stop_stream_inference(CRKitModel model);

/* ==================== 结果管理API ==================== */

/**
 * @brief 释放推理结果
 * @param result 推理结果指针
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_free_result(CRKitResult* result);

/**
 * @brief 释放批量推理结果
 * @param results 推理结果数组
 * @param num_results 结果数量
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_free_batch_results(CRKitResult** results,
                                               int num_results);

/**
 * @brief 释放图像数据
 * @param image 图像数据指针
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_free_image(CRKitImage* image);

/* ==================== 工具API ==================== */

/**
 * @brief 设置日志级别
 * @param level 日志级别: "DEBUG", "INFO", "WARN", "ERROR"
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_set_log_level(const char* level);

/**
 * @brief 设置日志输出文件
 * @param file_path 日志文件路径, NULL表示输出到控制台
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_set_log_file(const char* file_path);

/**
 * @brief 获取设备信息
 * @param device_type 设备类型
 * @param device_id 设备ID
 * @param info_buffer 信息缓冲区
 * @param buffer_size 缓冲区大小
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_get_device_info(CRKitDeviceType device_type,
                                            int device_id,
                                            char* info_buffer,
                                            int buffer_size);

/**
 * @brief 模型预热 (执行几次空推理以优化性能)
 * @param model 模型句柄
 * @param num_iterations 预热次数
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_warmup_model(CRKitModel model, int num_iterations);

/**
 * @brief 获取模型信息
 * @param model 模型句柄
 * @param info_buffer 信息缓冲区 (JSON格式)
 * @param buffer_size 缓冲区大小
 * @return 状态码
 */
CRKIT_API CRKitStatus crkit_get_model_info(CRKitModel model,
                                           char* info_buffer,
                                           int buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* CRKIT_API_H */
