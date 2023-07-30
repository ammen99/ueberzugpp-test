#include <memory>
#include <wayfire/core.hpp>
#include <wayfire/scene-render.hpp>
#include <wayfire/util.hpp>
#include <wayfire/view.hpp>
#include <wayfire/output.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/scene-operations.hpp>
#include <wayfire/plugins/ipc/ipc-method-repository.hpp>
#include <wayfire/plugins/common/shared-core-data.hpp>
#include <wayfire/unstable/wlr-view-events.hpp>
#include <wayfire/unstable/wlr-surface-node.hpp>
#include <wayfire/unstable/translation-node.hpp>

namespace wf
{
class uberzug_surface_t
{
    std::weak_ptr<wf::view_interface_t> terminal;
    std::shared_ptr<scene::wlr_surface_node_t> surface;
    std::shared_ptr<scene::translation_node_t> translation;
    wlr_xdg_toplevel *toplevel;

    wf::wl_listener_wrapper on_toplevel_destroy;

  public:

    uberzug_surface_t(std::weak_ptr<wf::view_interface_t> _terminal,
        wlr_xdg_toplevel *_toplevel)
    {
        this->terminal = _terminal;
        this->toplevel = _toplevel;

        // We create a custom surface node for the uberzug window and wrap it in a translation node, so that
        // we can control its position.
        // We add the translation node as a subsurface of the terminal.
        surface = std::make_shared<scene::wlr_surface_node_t>(_toplevel->base->surface, true);
        translation = std::make_shared<scene::translation_node_t>();

        translation->set_children_list({surface});

        auto term = _terminal.lock();
        wf::scene::add_front(term->get_surface_root_node(), translation);

        // Finally, wait for the toplevel to be destroyed
        on_toplevel_destroy.set_callback([this] (auto)
        {
            destroy_callback();
        });
        on_toplevel_destroy.connect(&toplevel->base->events.destroy);
    }

    std::function<void()> destroy_callback;

    ~uberzug_surface_t()
    {
        wf::scene::remove_child(translation);
    }

    void set_offset(int x, int y)
    {
        translation->set_offset({x, y});
        if (auto term = terminal.lock())
        {
            term->damage();
        }
    }
};


class uberzug_mapper : public wf::plugin_interface_t
{
  public:
    void init() override
    {
        ipc_repo->register_method("ueberzugpp/set_offset", set_offset);
        wf::get_core().connect(&on_new_xdg_surface);
    }

    void fini() override
    {
        ipc_repo->unregister_method("ueberzugpp/set_offset");
    }

    ipc::method_callback set_offset = [=] (nlohmann::json js)
    {
        WFJSON_EXPECT_FIELD(js, "app-id", string);
        WFJSON_EXPECT_FIELD(js, "x", number_integer);
        WFJSON_EXPECT_FIELD(js, "y", number_integer);

        const std::string app_id = js["app-id"];
        const int x = js["x"];
        const int y = js["y"];

        if (uberzugs.find(app_id) == uberzugs.end())
        {
            return ipc::json_error("Unknown uberzug window with appid " + app_id);
        }

        uberzugs[app_id]->set_offset(x, y);
        return ipc::json_ok();
    };

    shared_data::ref_ptr_t<ipc::method_repository_t> ipc_repo;

    // When a new xdg_toplevel is created, we need to check whether it is an uberzug window by looking at
    // its app-id. If it is indeed an uberzug window, we take over the toplevel (by setting
    // use_default_implementation=false) and create our own uberzug_surface. In addition, we directly map
    // the surface to the currently focused view, if it exists.
    wf::signal::connection_t<new_xdg_surface_signal> on_new_xdg_surface = [=] (new_xdg_surface_signal *ev)
    {
        if (!ev->use_default_implementation) return;
        if (ev->surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) return;

        std::string appid = ev->surface->toplevel->title ?: "null";
        const std::string search_for = "ueberzugpp_";
        if (appid.find(search_for) == 0)
        {
            auto terminal = wf::get_core().get_active_output()->get_active_view();
            if (!terminal)
            {
                LOGE("Which window to map uberzug to????");
                return;
            }

            ev->use_default_implementation = false;
            uberzugs[appid] = std::make_unique<uberzug_surface_t>(
                terminal->weak_from_this(), ev->surface->toplevel);

            uberzugs[appid]->destroy_callback = [this, appid]
            {
                uberzugs.erase(appid);
            };
        }
    };

    std::map<std::string, std::unique_ptr<uberzug_surface_t>> uberzugs;
};
}

DECLARE_WAYFIRE_PLUGIN(wf::uberzug_mapper);
