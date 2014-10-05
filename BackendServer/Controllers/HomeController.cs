using BackendServer.Models;
using System;
using System.Collections.Generic;
using System.Linq;
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
            if (!string.IsNullOrEmpty(model.FileHostCode))
            {
                ModelState.AddModelError("errorSummary", "The specified file host doesn't exist.");
            }

            return View(model);
        }
    }
}