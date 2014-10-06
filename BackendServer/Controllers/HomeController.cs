using BackendServer.Models;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Web;
using System.Web.Mvc;

namespace BackendServer.Controllers
{
    public class HomeController : Controller
    {
        public ApplicationDatabase database = new ApplicationDatabase();

        public ActionResult Index()
        {
            return View(new FileHostModel());
        }

        [HttpPost]
        [AllowAnonymous]
        public ActionResult Index(FileHostModel model)
        {
            if (!ModelState.IsValid)
            {
                return View(model);
            }

            var host = database.Hosts.Where(x => x.HostCode == model.FileHostCode).SingleOrDefault();

            if (host != null && host.Ip != null)
            {
                return Redirect(host.Ip);
            }

            ModelState.AddModelError("errorSummary", "The specified file host doesn't exist.");
            return View(model);
        }

        [HttpPost]
        [AllowAnonymous]
        public ActionResult RegisterConnection(UserIdentityModel userIdentity)
        {
            if (Debugger.IsAttached)
            {
                Debugger.Break();
            }

            return null;
        }
    }    
}