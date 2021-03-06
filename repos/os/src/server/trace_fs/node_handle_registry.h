/*
 * \brief  Facility for managing the session-local node-handle namespace
 * \author Norman Feske
 * \date   2012-04-11
 */

#ifndef _NODE_HANDLE_REGISTRY_H_
#define _NODE_HANDLE_REGISTRY_H_

namespace File_system {

	class Node;
	class Directory;
	class File;
	class Symlink;

	/**
	 * Type trait for determining the node type for a given handle type
	 */
	template<typename T> struct Node_type;
	template<> struct Node_type<Node_handle>    { typedef Node      Type; };
	template<> struct Node_type<Dir_handle>     { typedef Directory Type; };
	template<> struct Node_type<File_handle>    { typedef File      Type; };
	template<> struct Node_type<Symlink_handle> { typedef Symlink   Type; };


	/**
	 * Type trait for determining the handle type for a given node type
	 */
	template<typename T> struct Handle_type;
	template<> struct Handle_type<Node>      { typedef Node_handle    Type; };
	template<> struct Handle_type<Directory> { typedef Dir_handle     Type; };
	template<> struct Handle_type<File>      { typedef File_handle    Type; };
	template<> struct Handle_type<Symlink>   { typedef Symlink_handle Type; };


	class Node_handle_registry
	{
		private:

			/* maximum number of open nodes per session */
			enum { MAX_NODE_HANDLES = 128U };

			Lock mutable _lock;

			Node *_nodes[MAX_NODE_HANDLES];

			/**
			 * Each open node handle can act as a listener to be informed about
			 * node changes.
			 */
			Listener _listeners[MAX_NODE_HANDLES];

			/**
			 * Allocate node handle
			 *
			 * \throw Out_of_node_handles
			 */
			int _alloc(Node *node)
			{
				Lock::Guard guard(_lock);

				for (unsigned i = 0; i < MAX_NODE_HANDLES; i++)
					if (!_nodes[i]) {
						_nodes[i] = node;
						return i;
					}

				throw Out_of_node_handles();
			}

			bool _in_range(int handle) const
			{
				return ((handle >= 0) && (handle < MAX_NODE_HANDLES));
			}

		public:

			Node_handle_registry()
			{
				for (unsigned i = 0; i < MAX_NODE_HANDLES; i++)
					_nodes[i] = 0;
			}

			template <typename NODE_TYPE>
			typename Handle_type<NODE_TYPE>::Type alloc(NODE_TYPE *node)
			{
				typedef typename Handle_type<NODE_TYPE>::Type Handle;
				return Handle(_alloc(node));
			}

			/**
			 * Release node handle
			 */
			void free(Node_handle handle)
			{
				if (!_in_range(handle.value))
					return;

				/*
				 * Notify listeners about the changed file.
				 */
				Node *node = dynamic_cast<Node *>(_nodes[handle.value]);
				if (!node) { return; }

				node->notify_listeners();

				/*
				 * De-allocate handle
				 */
				Listener &listener = _listeners[handle.value];

				if (listener.valid())
					node->remove_listener(&listener);

				_nodes[handle.value] = 0;
				listener = Listener();
			}

			/**
			 * Lookup node using its handle as key
			 *
			 * \throw Invalid_handle
			 */
			template <typename HANDLE_TYPE>
			typename Node_type<HANDLE_TYPE>::Type *lookup(HANDLE_TYPE handle)
			{
				if (!_in_range(handle.value))
					throw Invalid_handle();

				typedef typename Node_type<HANDLE_TYPE>::Type Node;
				Node *node = dynamic_cast<Node *>(_nodes[handle.value]);
				if (!node)
					throw Invalid_handle();

				return node;
			}

			bool refer_to_same_node(Node_handle h1, Node_handle h2) const
			{
				if (!_in_range(h1.value) || !_in_range(h2.value)) {
					PDBG("refer_to_same_node -> Invalid_handle");
					throw Invalid_handle();
				}

				return _nodes[h1.value] == _nodes[h2.value];
			}

			/**
			 * Register signal handler to be notified of node changes
			 */
			void sigh(Node_handle handle, Signal_context_capability sigh)
			{
				if (!_in_range(handle.value))
					throw Invalid_handle();

				Node *node = dynamic_cast<Node *>(_nodes[handle.value]);
				if (!node) {
					PDBG("Invalid_handle");
					throw Invalid_handle();
				}

				Listener &listener = _listeners[handle.value];

				/*
				 * If there was already a handler registered for the node,
				 * remove the old handler.
				 */
				if (listener.valid())
					node->remove_listener(&listener);

				/*
				 * Register new handler
				 */
				listener = Listener(sigh);
				node->add_listener(&listener);
			}
	};
}

#endif /* _NODE_HANDLE_REGISTRY_H_ */
