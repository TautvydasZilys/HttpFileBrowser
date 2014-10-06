using BackendServer.Models;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Web;
using System.Web.Mvc;

namespace BackendServer.Controllers
{
    public class HomeController : Controller
    {
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

            /*var host = database.Hosts.Where(x => x.HostUniqueId == model.FileHostCode).SingleOrDefault();

            if (host != null && host.Ip != 0)
            {
                return Redirect(new IPEndPoint(host.Ip, host.Port).ToString());
            }
            */
            ModelState.AddModelError("errorSummary", "The specified file host doesn't exist.");
            return View(model);
        }
    }    
}