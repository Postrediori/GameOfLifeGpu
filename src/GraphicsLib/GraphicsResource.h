#pragma once

namespace GraphicsUtils {

    struct unique_any {
        unique_any(unique_any const&) = delete; // No copy constructor
        unique_any& operator=(unique_any const&) = delete; // No copy operator

        unique_any() = default;

        virtual ~unique_any() {
            reset();
        }

        // Copying constructor
        unique_any(unique_any&& other)
            : resourceId_(other.resourceId_) {
            other.resourceId_ = 0;
        }

        // Swap resources between smart pointers
        auto swap(unique_any& other) -> void {
            std::swap(resourceId_, other.resourceId_);
        }

        auto swap(unique_any& left, unique_any& right) -> void {
            left.swap(right);
        }

        // Retireve the resource
        auto get() const -> GLuint {
            return resourceId_;
        }

        // Same as previous
        explicit operator GLuint() const {
            return get();
        }

        // Check validity of the resource
        auto is_valid() const -> bool {
            return (resourceId_ != 0);
        }

        // Same as previous
        explicit operator bool() const {
            return is_valid();
        }

        // Free or free+replace the resource
        auto reset(GLuint textureId = 0) -> void {
            if (is_valid()) {
                close();
            }
            resourceId_ = textureId;
        }

        // Detach resource from the pointer without freeing
        auto release() -> GLuint {
            auto t = resourceId_;
            resourceId_ = 0;
            return t;
        }

        // Return the address of the internal resource for out parameter use
        auto addressof() -> GLuint* {
            return &resourceId_;
        }

        // Same as previous but also frees any currently-held resource
        auto put() -> GLuint* {
            reset();
            return addressof();
        }

        // Same as previous
        GLuint* operator&() {
            return put();
        }

        // Actual deallocation of resource
        virtual auto close() -> void { }

        GLuint resourceId_{ 0 };
    };

    struct unique_texture : public unique_any {
        auto close() -> void;
    };

    struct unique_framebuffer : public unique_any {
        auto close() -> void;
    };

    struct unique_program : public unique_any {
        auto close() -> void;
    };

    struct unique_vertex_array : public unique_any {
        auto close() -> void;
    };

    struct unique_buffer : public unique_any {
        auto close() -> void;
    };
}
