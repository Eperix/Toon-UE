import { projectStore } from "../../backend/ProjectStore";


export const getActiveStreamId = (): string | undefined => {

    const path = window.location.pathname?.split("/").filter(c => !!c.trim());
    if (path.length < 2) {
        return undefined;
    }

    if (path[0] !== "stream") {
        return undefined;
    }

    if (!projectStore.streamById(path[1])) {
        return undefined;
    }

    return path[1];

}