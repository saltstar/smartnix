
#include <trace/event.h>

#include <zircon/assert.h>
#include <zircon/syscalls.h>

namespace {

struct EventHelper {
    EventHelper(trace_context_t* context, const char* name_literal)
        : ticks(zx_ticks_get()) {
        trace_context_register_current_thread(context, &thread_ref);
        trace_context_register_string_literal(context, name_literal, &name_ref);
    }

    trace_ticks_t const ticks;
    trace_thread_ref_t thread_ref;
    trace_string_ref_t name_ref;
};

struct VThreadEventHelper {
    VThreadEventHelper(trace_context_t* context,
                       const char* name_literal,
                       const char* vthread_literal,
                       trace_vthread_id_t vthread_id)
        : ticks(zx_ticks_get()) {
        trace_context_register_vthread(
            context, ZX_KOID_INVALID, vthread_literal, vthread_id, &thread_ref);
        trace_context_register_string_literal(context, name_literal, &name_ref);
    }

    trace_ticks_t const ticks;
    trace_thread_ref_t thread_ref;
    trace_string_ref_t name_ref;
};

} // namespace

void trace_internal_write_instant_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    trace_scope_t scope,
    const trace_arg_t* args, size_t num_args) {
    EventHelper helper(context, name_literal);
    trace_context_write_instant_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        scope, args, num_args);
    trace_release_context(context);
}

void trace_internal_write_counter_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    trace_counter_id_t counter_id,
    const trace_arg_t* args, size_t num_args) {
    EventHelper helper(context, name_literal);
    trace_context_write_counter_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        counter_id, args, num_args);
    trace_release_context(context);
}

void trace_internal_write_duration_begin_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    const trace_arg_t* args, size_t num_args) {
    EventHelper helper(context, name_literal);
    trace_context_write_duration_begin_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        args, num_args);
    trace_release_context(context);
}

void trace_internal_write_duration_end_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    const trace_arg_t* args, size_t num_args) {
    EventHelper helper(context, name_literal);
    trace_context_write_duration_end_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        args, num_args);
    trace_release_context(context);
}

void trace_internal_write_async_begin_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    trace_async_id_t async_id,
    const trace_arg_t* args, size_t num_args) {
    EventHelper helper(context, name_literal);
    trace_context_write_async_begin_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        async_id, args, num_args);
    trace_release_context(context);
}

void trace_internal_write_async_instant_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    trace_async_id_t async_id,
    const trace_arg_t* args, size_t num_args) {
    EventHelper helper(context, name_literal);
    trace_context_write_async_instant_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        async_id, args, num_args);
    trace_release_context(context);
}

void trace_internal_write_async_end_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    trace_async_id_t async_id,
    const trace_arg_t* args, size_t num_args) {
    EventHelper helper(context, name_literal);
    trace_context_write_async_end_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        async_id, args, num_args);
    trace_release_context(context);
}

void trace_internal_write_flow_begin_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    trace_flow_id_t flow_id,
    const trace_arg_t* args, size_t num_args) {
    EventHelper helper(context, name_literal);
    trace_context_write_flow_begin_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        flow_id, args, num_args);
    trace_release_context(context);
}

void trace_internal_write_flow_step_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    trace_flow_id_t flow_id,
    const trace_arg_t* args, size_t num_args) {
    EventHelper helper(context, name_literal);
    trace_context_write_flow_step_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        flow_id, args, num_args);
    trace_release_context(context);
}

void trace_internal_write_flow_end_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    trace_flow_id_t flow_id,
    const trace_arg_t* args, size_t num_args) {
    EventHelper helper(context, name_literal);
    trace_context_write_flow_end_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        flow_id, args, num_args);
    trace_release_context(context);
}

void trace_internal_write_kernel_object_record_for_handle_and_release_context(
    trace_context_t* context,
    zx_handle_t handle,
    const trace_arg_t* args, size_t num_args) {
    trace_context_write_kernel_object_record_for_handle(
        context, handle, args, num_args);
    trace_release_context(context);
}

void trace_internal_write_blob_record_and_release_context(
    trace_context_t* context,
    trace_blob_type_t type,
    const char* name_literal,
    const void* blob, size_t blob_size) {
    trace_string_ref_t name_ref;
    trace_context_register_string_literal(context, name_literal, &name_ref);
    trace_context_write_blob_record(
        context, type, &name_ref, blob, blob_size);
    trace_release_context(context);
}

void trace_internal_write_vthread_duration_begin_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    const char* vthread_literal,
    trace_vthread_id_t vthread_id,
    const trace_arg_t* args, size_t num_args) {
    VThreadEventHelper helper(context, name_literal, vthread_literal, vthread_id);
    trace_context_write_duration_begin_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        args, num_args);
    trace_release_context(context);
}

void trace_internal_write_vthread_duration_end_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    const char* vthread_literal,
    trace_vthread_id_t vthread_id,
    const trace_arg_t* args, size_t num_args) {
    VThreadEventHelper helper(context, name_literal, vthread_literal, vthread_id);
    trace_context_write_duration_end_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        args, num_args);
    trace_release_context(context);
}

void trace_internal_write_vthread_flow_begin_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    const char* vthread_literal,
    trace_vthread_id_t vthread_id,
    trace_flow_id_t flow_id,
    const trace_arg_t* args, size_t num_args) {
    VThreadEventHelper helper(context, name_literal, vthread_literal, vthread_id);
    trace_context_write_flow_begin_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        flow_id, args, num_args);
    trace_release_context(context);
}

void trace_internal_write_vthread_flow_step_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    const char* vthread_literal,
    trace_vthread_id_t vthread_id,
    trace_flow_id_t flow_id,
    const trace_arg_t* args, size_t num_args) {
    VThreadEventHelper helper(context, name_literal, vthread_literal, vthread_id);
    trace_context_write_flow_step_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        flow_id, args, num_args);
    trace_release_context(context);
}

void trace_internal_write_vthread_flow_end_event_record_and_release_context(
    trace_context_t* context,
    const trace_string_ref_t* category_ref,
    const char* name_literal,
    const char* vthread_literal,
    trace_vthread_id_t vthread_id,
    trace_flow_id_t flow_id,
    const trace_arg_t* args, size_t num_args) {
    VThreadEventHelper helper(context, name_literal, vthread_literal, vthread_id);
    trace_context_write_flow_end_event_record(
        context, helper.ticks, &helper.thread_ref, category_ref, &helper.name_ref,
        flow_id, args, num_args);
    trace_release_context(context);
}
