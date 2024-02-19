/* This file is part of Daedalus Turbo project: https://github.com/sierkov/daedalus-turbo/
 * Copyright (c) 2022-2024 Alex Sierkov (alex dot sierkov at gmail dot com)
 * This code is distributed under the license specified in:
 * https://github.com/sierkov/daedalus-turbo/blob/main/LICENSE */
#ifndef DAEDALUS_TURBO_INDEX_MERGE_HPP
#define DAEDALUS_TURBO_INDEX_MERGE_HPP

#include <dt/index/io.hpp>

namespace daedalus_turbo::index {
    template<typename T>
    inline uint64_t merge_index_part(index::writer<T> &out_idx, size_t part_idx, const std::vector<std::shared_ptr<index::reader_mt<T>>> &readers)
    {
        std::vector<typename index::reader_mt<T>::thread_data> reader_data {};
        merge_queue<T> items_to_consider {};
        uint64_t max_offset = 0;
        for (size_t i = 0; i < readers.size(); ++i) {
            reader_data.emplace_back(readers[i]->init_thread(part_idx));
            T val;
            if (readers[i]->read_part(part_idx, val, reader_data[i]))
                items_to_consider.emplace(std::move(val), i);
            uint64_t r_max_offset = readers[i]->get_meta("max_offset").template to<uint64_t>();
            if (r_max_offset > max_offset)
                max_offset = r_max_offset;
        }
        while (items_to_consider.size() > 0) {
            merge_item next { items_to_consider.top() };
            items_to_consider.pop();
            out_idx.emplace_part(part_idx, next.val);
            if (readers[next.stream_idx]->read_part(part_idx, next.val, reader_data[next.stream_idx]))
                items_to_consider.emplace(std::move(next));
        }
        return max_offset;
    }

    template<typename T>
    inline size_t merge_estimate_task_count(const std::vector<std::string> &chunks)
    {
        if (chunks.empty())
            return 0;
        else if (chunks.size() == 1)
            return 1;
        index::reader_mt<T> reader { chunks.at(0) };
        return reader.num_parts();
    }

    template<typename T>
    inline void merge_one_step(scheduler &sched, const std::string &task_group, size_t task_prio, const std::vector<std::string> &chunks, const std::string &final_path)
    {
        if (chunks.size() == 1) {
            auto chunk = chunks.at(0);
            sched.submit_void(task_group, task_prio, [=]() {
                index::writer<int>::rename(chunk, final_path);
                logger::trace("merged {} chunks into {}", chunk, final_path);
            });
        } else if (!chunks.empty()) {
            std::vector<std::shared_ptr<index::reader_mt<T>>> readers {};
            size_t num_parts = 0;
            for (size_t i = 0; i < chunks.size(); ++i) {
                auto &reader = readers.emplace_back(std::make_shared<index::reader_mt<T>>(chunks[i]));
                if (num_parts == 0)
                    num_parts = reader->num_parts();
                if (num_parts != reader->num_parts())
                    throw error("chunk {} has a partition count: {} different from the one found in other chunks: {}!",
                            chunks[i], reader->num_parts(), num_parts);
            }
            auto out_idx = std::make_shared<index::writer<T>>(final_path, num_parts);
            auto todo_parts = std::make_shared<std::atomic_size_t>(num_parts);
            auto done_parts = std::make_shared<std::atomic_size_t>(0);
            auto max_offset = std::make_shared<std::atomic<uint64_t>>(0);
            sched.on_result(task_group, [todo_parts, done_parts, max_offset, out_idx, task_group, chunks, final_path](const auto &res) mutable {
                if (res.type() == typeid(scheduled_task_error)) {
                    logger::error("task {} {}", task_group, std::any_cast<scheduled_task_error>(res).what());
                    return;
                }
                auto part_max_offset = std::any_cast<uint64_t>(res);
                if (part_max_offset > *max_offset)
                    *max_offset = part_max_offset;
                if (++*done_parts >= *todo_parts) {
                    out_idx->set_meta("max_offset", buffer::from(*max_offset));
                    out_idx->commit();
                    // all readers must be already closed since all tasks refering to the shared_ptr are finished
                    for (const auto &path: chunks)
                        index::writer<T>::remove(path);
                    logger::trace("merged {} chunks into {}", chunks.size(), final_path);
                }
            });
            for (size_t pi = 0; pi < num_parts; ++pi) {
                sched.submit(task_group, task_prio, [=]() {
                    return merge_index_part(*out_idx, pi, readers);
                });
            }
        }
    }
}

#endif //!DAEDALUS_TURBO_INDEX_MERGE_HPP