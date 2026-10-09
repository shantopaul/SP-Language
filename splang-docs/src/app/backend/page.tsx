import { redirect } from "next/navigation";

export default function BackendRootPage() {
  redirect("/backend/overview");
}
