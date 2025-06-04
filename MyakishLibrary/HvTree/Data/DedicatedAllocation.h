#pragma once
#include <HvTree2/HvTree.h>

namespace hv::data
{
    class DedicatedAllocationEntry
    {
        std::byte* data;
        std::size_t size;

        template<typename>
        friend class DedicatedAllocationStorage;

    public:

        DedicatedAllocationEntry() : data(nullptr), size(0)
        {

        }
        DedicatedAllocationEntry(std::size_t size) : data(nullptr), size(0)
        {
            data = new std::byte[size];
            this->size = size;
        }

        DedicatedAllocationEntry(const DedicatedAllocationEntry&) = delete;
        DedicatedAllocationEntry(DedicatedAllocationEntry&& rhs) noexcept : data(rhs.data), size(rhs.size)
        {
            rhs.data = nullptr;
            rhs.size = 0;
        }

        void Destroy()
        {
            if (data)
            {
                delete[] data;
                size = 0;
            }
        }

        myakish::streams::ConstRaw Read() const
        {
            return myakish::streams::ConstRaw(data, size);
        }

        struct EntryOutputStream
        {
            DedicatedAllocationEntry& entry;

            EntryOutputStream(DedicatedAllocationEntry& entry) : entry(entry) {}

            bool Reserve(myakish::Size reserve)
            {
                entry.Destroy();
                entry.data = new std::byte[reserve];
                entry.size = reserve;
                return true;
            }

            myakish::streams::RawIterator Begin()
            {
                return myakish::streams::RawIterator(entry.data, entry.data + entry.size);
            }
        };
        static_assert(myakish::streams::BinaryOutput<EntryOutputStream>, "hv::DedicatedAllocationStorage::Entry::EntryOutputStream should satisfy myakish::streams::BinaryOutput");

        DedicatedAllocationEntry& operator=(const DedicatedAllocationEntry&) = delete;
        DedicatedAllocationEntry& operator=(DedicatedAllocationEntry&& rhs) noexcept
        {
            std::swap(data, rhs.data);
            std::swap(size, rhs.size);
            return *this;
        }

        EntryOutputStream Write()
        {
            return EntryOutputStream(*this);
        }

        ~DedicatedAllocationEntry()
        {
            Destroy();
        }
    };
    static_assert(Entry<DedicatedAllocationEntry>, "hv::DedicatedAllocationEntry should satisfy hv::data::Entry");

    template<typename Handle>
    class DedicatedAllocationStorage
    {
    public:

        using NullHandle = Handle;
        using HandleFamily = handle::HandleFamily<Handle>;
        using Entry = DedicatedAllocationEntry;

    private:

        std::map<Handle, Entry> entries;

    public:

        Entry& Acquire(Handle handle)
        {
            return entries[handle];
        }
        bool Exists(Handle handle)
        {
            return entries.contains(handle);
        }

        template<myakish::streams::BinaryOutput OutputStream>
        void Save(OutputStream&& out)
        {
            auto count = entries.size();
            auto structureSize = count * (sizeof(Handle) + sizeof(myakish::Size/*size*/));
            auto dataSize = std::ranges::fold_left_first(entries | std::views::values | std::views::transform([](const auto& entry) { return entry.size; }), std::plus{}).value();

            auto totalSize = structureSize + dataSize + sizeof(myakish::Size/*count*/);

            out.Reserve(totalSize);

            auto it = out.Begin();
            myakish::streams::Write(it, count);

            for (auto& [handle, entry] : entries)
            {
                myakish::streams::Write(it, handle);
                myakish::streams::Write(it, entry.size);
                it.Write(entry.data, entry.size);
            }
        }

        template<myakish::streams::BinaryInput InputStream>
        void Load(InputStream&& in)
        {
            auto it = in.Begin();
            auto count = myakish::streams::Read<myakish::Size>(it);

            for (auto i = 0; i < count; i++)
            {
                auto handle = myakish::streams::Read<Handle>(it);
                auto size = myakish::streams::Read<std::size_t>(it);
                auto [entryIt, inserted] = entries.emplace(handle, size);
                it.Read(entryIt->second.data, size);
            }
        }
    };

}