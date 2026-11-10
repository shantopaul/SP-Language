module routes;

import std.web;
import handlers;

// ============================================================================
// Central route table for the SP registry service.
//
// This file is the single source of truth for the service's routes: the
// OpenAPI spec (openapi.yaml) is GENERATED from these registrations by
// generate_openapi.py, which reads the // @openapi annotations below.
// Keep each annotation directly above its registration, and keep the
// annotation in sync with the route it describes.
//
// Annotation fields:
//   @openapi summary: <one-line description>
//   @openapi auth: none | bearer
//   @openapi param: <name> path <type>   (one line per path parameter)
// ============================================================================

public class RegistryRoutes {
    public static fn registerRoutes(App app) {
        // @openapi summary: Service health probe (also reports stored package count)
        // @openapi auth: none
        app.get("/api/health", (ptr req) => {
            let ctx = Context.create(req);
            return RegistryHandlers.health(ctx);
        });

        // @openapi summary: Publish package metadata (what spm publish sends)
        // @openapi auth: bearer
        app.post("/api/v1/publish", (ptr req) => {
            let ctx = Context.create(req);
            return RegistryHandlers.publish(ctx);
        });

        // @openapi summary: Upload the base64 tarball for a published version
        // @openapi auth: bearer
        // @openapi param: name path string
        // @openapi param: version path string
        app.post("/api/v1/packages/:name/:version/tarball", (ptr req) => {
            let ctx = Context.create(req);
            return RegistryHandlers.uploadTarball(ctx);
        });

        // @openapi summary: Verify a package version against stored data
        // @openapi auth: none
        // @openapi param: name path string
        // @openapi param: version path string
        app.get("/api/v1/verify/:name/:version", (ptr req) => {
            let ctx = Context.create(req);
            return RegistryHandlers.verify(ctx);
        });

        // @openapi summary: List all published packages
        // @openapi auth: none
        app.get("/api/v1/packages", (ptr req) => {
            let ctx = Context.create(req);
            return RegistryHandlers.listPackages(ctx);
        });

        // @openapi summary: Get metadata for one package version
        // @openapi auth: none
        // @openapi param: name path string
        // @openapi param: version path string
        app.get("/api/v1/packages/:name/:version", (ptr req) => {
            let ctx = Context.create(req);
            return RegistryHandlers.packageInfo(ctx);
        });

        // @openapi summary: Download the tarball for one package version (base64 body)
        // @openapi auth: none
        // @openapi param: name path string
        // @openapi param: version path string
        app.get("/api/v1/packages/:name/:version/download", (ptr req) => {
            let ctx = Context.create(req);
            return RegistryHandlers.download(ctx);
        });

        // @openapi summary: OpenAPI specification for this service (YAML)
        // @openapi auth: none
        app.get("/openapi.yaml", (ptr req) => {
            let ctx = Context.create(req);
            return RegistryHandlers.openApi(ctx);
        });
    }
}
